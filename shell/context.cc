// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/context.h"

#include <vector>

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "mojo/common/trace_controller_impl.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/simple_platform_support.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "services/tracing/tracing.mojom.h"
#include "shell/application_manager/application_loader.h"
#include "shell/application_manager/application_manager.h"
#include "shell/command_line_util.h"
#include "shell/external_application_listener.h"
#include "shell/filename_util.h"
#include "shell/in_process_dynamic_service_runner.h"
#include "shell/out_of_process_dynamic_service_runner.h"
#include "shell/switches.h"
#include "url/gurl.h"

namespace mojo {
namespace shell {
namespace {

// Used to ensure we only init once.
class Setup {
 public:
  Setup() {
    embedder::Init(scoped_ptr<mojo::embedder::PlatformSupport>(
        new mojo::embedder::SimplePlatformSupport()));
  }

  ~Setup() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Setup);
};

void InitContentHandlers(ApplicationManager* manager,
                         base::CommandLine* command_line) {
  // Default content handlers.
  manager->RegisterContentHandler("application/pdf", GURL("mojo:pdf_viewer"));
  manager->RegisterContentHandler("image/png", GURL("mojo:png_viewer"));
  manager->RegisterContentHandler("text/html", GURL("mojo:html_viewer"));

  // Command-line-specified content handlers.
  std::string handlers_spec =
      command_line->GetSwitchValueASCII(switches::kContentHandlers);
  if (handlers_spec.empty())
    return;

#if defined(OS_ANDROID)
  // TODO(eseidel): On Android we pass command line arguments is via the
  // 'parameters' key on the intent, which we specify during 'am shell start'
  // via --esa, however that expects comma-separated values and says:
  //   am shell --help:
  //     [--esa <EXTRA_KEY> <EXTRA_STRING_VALUE>[,<EXTRA_STRING_VALUE...]]
  //     (to embed a comma into a string escape it using "\,")
  // Whatever takes 'parameters' and constructs a CommandLine is failing to
  // un-escape the commas, we need to move this fix to that file.
  ReplaceSubstringsAfterOffset(&handlers_spec, 0, "\\,", ",");
#endif

  std::vector<std::string> parts;
  base::SplitString(handlers_spec, ',', &parts);
  if (parts.size() % 2 != 0) {
    LOG(ERROR) << "Invalid value for switch " << switches::kContentHandlers
               << ": must be a comma-separated list of mimetype/url pairs."
               << handlers_spec;
    return;
  }

  for (size_t i = 0; i < parts.size(); i += 2) {
    GURL url(parts[i + 1]);
    if (!url.is_valid()) {
      LOG(ERROR) << "Invalid value for switch " << switches::kContentHandlers
                 << ": '" << parts[i + 1] << "' is not a valid URL.";
      return;
    }
    // TODO(eseidel): We should also validate that the mimetype is valid
    // net/base/mime_util.h could do this, but we don't want to depend on net.
    manager->RegisterContentHandler(parts[i], url);
  }
}

bool ConfigureURLMappings(base::CommandLine* command_line, Context* context) {
  URLResolver* resolver = context->url_resolver();

  // Configure the resolution of unknown mojo: URLs.
  GURL base_url;
  if (command_line->HasSwitch(switches::kOrigin))
    base_url = GURL(command_line->GetSwitchValueASCII(switches::kOrigin));
  else
    // Use the shell's file root if the base was not specified.
    base_url = context->ResolveShellFileURL("");

  if (!base_url.is_valid())
    return false;

  resolver->SetMojoBaseURL(base_url);

  // The network service must be loaded from the filesystem.
  // This mapping is done before the command line URL mapping are processed, so
  // that it can be overridden.
  resolver->AddURLMapping(
      GURL("mojo:network_service"),
      context->ResolveShellFileURL("file:network_service.mojo"));

  // Command line URL mapping.
  std::vector<URLResolver::OriginMapping> origin_mappings =
      URLResolver::GetOriginMappings(command_line->argv());
  for (const auto& origin_mapping : origin_mappings)
    resolver->AddOriginMapping(GURL(origin_mapping.origin),
                               GURL(origin_mapping.base_url));

  if (command_line->HasSwitch(switches::kURLMappings)) {
    const std::string mappings =
        command_line->GetSwitchValueASCII(switches::kURLMappings);

    base::StringPairs pairs;
    if (!base::SplitStringIntoKeyValuePairs(mappings, '=', ',', &pairs))
      return false;
    using StringPair = std::pair<std::string, std::string>;
    for (const StringPair& pair : pairs) {
      const GURL from(pair.first);
      const GURL to = context->ResolveCommandLineURL(pair.second);
      if (!from.is_valid() || !to.is_valid())
        return false;
      resolver->AddURLMapping(from, to);
    }
  }
  return true;
}

class TracingServiceProvider : public ServiceProvider {
 public:
  explicit TracingServiceProvider(InterfaceRequest<ServiceProvider> request)
      : binding_(this, request.Pass()) {}
  ~TracingServiceProvider() override {}

  void ConnectToService(const mojo::String& service_name,
                        ScopedMessagePipeHandle client_handle) override {
    if (service_name == tracing::TraceController::Name_) {
      new TraceControllerImpl(
          MakeRequest<tracing::TraceController>(client_handle.Pass()));
    }
  }

 private:
  StrongBinding<ServiceProvider> binding_;

  DISALLOW_COPY_AND_ASSIGN(TracingServiceProvider);
};

}  // namespace

Context::Context() : application_manager_(this) {
  DCHECK(!base::MessageLoop::current());

  // By default assume that the local apps reside alongside the shell.
  // TODO(ncbray): really, this should be passed in rather than defaulting.
  // This default makes sense for desktop but not Android.
  base::FilePath shell_dir;
  PathService::Get(base::DIR_MODULE, &shell_dir);
  SetShellFileRoot(shell_dir);

  base::FilePath cwd;
  PathService::Get(base::DIR_CURRENT, &cwd);
  SetCommandLineCWD(cwd);
}

Context::~Context() {
  DCHECK(!base::MessageLoop::current());
}

// static
void Context::EnsureEmbedderIsInitialized() {
  static base::LazyInstance<Setup>::Leaky setup = LAZY_INSTANCE_INITIALIZER;
  setup.Get();
}

void Context::SetShellFileRoot(const base::FilePath& path) {
  shell_file_root_ = AddTrailingSlashIfNeeded(FilePathToFileURL(path));
}

GURL Context::ResolveShellFileURL(const std::string& path) {
  return shell_file_root_.Resolve(path);
}

void Context::SetCommandLineCWD(const base::FilePath& path) {
  command_line_cwd_ = AddTrailingSlashIfNeeded(FilePathToFileURL(path));
}

GURL Context::ResolveCommandLineURL(const std::string& path) {
  return command_line_cwd_.Resolve(path);
}

bool Context::Init() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  if (command_line->HasSwitch(switches::kWaitForDebugger))
    base::debug::WaitForDebugger(60, true);

  EnsureEmbedderIsInitialized();
  task_runners_.reset(
      new TaskRunners(base::MessageLoop::current()->message_loop_proxy()));

  // TODO(vtl): Probably these failures should be checked before |Init()|, and
  // this function simply shouldn't fail.
  if (!shell_file_root_.is_valid())
    return false;
  if (!ConfigureURLMappings(command_line, this))
    return false;

  // TODO(vtl): This should be MASTER, not NONE.
  embedder::InitIPCSupport(
      embedder::ProcessType::NONE, task_runners_->shell_runner(), this,
      task_runners_->io_runner(), embedder::ScopedPlatformHandle());

  if (command_line->HasSwitch(switches::kEnableExternalApplications)) {
    listener_.reset(new ExternalApplicationListener(
        task_runners_->shell_runner(), task_runners_->io_runner()));

    base::FilePath socket_path =
        command_line->GetSwitchValuePath(switches::kEnableExternalApplications);
    if (socket_path.empty())
      socket_path = ExternalApplicationListener::ConstructDefaultSocketPath();

    listener_->ListenInBackground(
        socket_path,
        base::Bind(&ApplicationManager::RegisterExternalApplication,
                   base::Unretained(&application_manager_)));
  }

  scoped_ptr<NativeRunnerFactory> runner_factory;
  if (command_line->HasSwitch(switches::kEnableMultiprocess))
    runner_factory.reset(new OutOfProcessDynamicServiceRunnerFactory(this));
  else
    runner_factory.reset(new InProcessDynamicServiceRunnerFactory(this));
  application_manager_.set_blocking_pool(task_runners_->blocking_pool());
  application_manager_.set_native_runner_factory(runner_factory.Pass());
  application_manager_.set_disable_cache(
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableCache));

  InitContentHandlers(&application_manager_, command_line);

  ServiceProviderPtr tracing_service_provider_ptr;
  new TracingServiceProvider(GetProxy(&tracing_service_provider_ptr));
  application_manager_.ConnectToApplication(
      GURL("mojo:tracing"), GURL(""), nullptr,
      tracing_service_provider_ptr.Pass());

  if (listener_)
    listener_->WaitForListening();

  return true;
}

void Context::Shutdown() {
  DCHECK_EQ(base::MessageLoop::current()->task_runner(),
            task_runners_->shell_runner());
  embedder::ShutdownIPCSupport();
  // We'll quit when we get OnShutdownComplete().
  base::MessageLoop::current()->Run();
}

void Context::OnApplicationError(const GURL& url) {
  if (app_urls_.find(url) != app_urls_.end()) {
    app_urls_.erase(url);
    if (app_urls_.empty() && base::MessageLoop::current()->is_running()) {
      DCHECK_EQ(base::MessageLoop::current()->task_runner(),
                task_runners_->shell_runner());
      base::MessageLoop::current()->Quit();
    }
  }
}

GURL Context::ResolveURL(const GURL& url) {
  return url_resolver_.ResolveMojoURL(url);
}

GURL Context::ResolveMappings(const GURL& url) {
  return url_resolver_.ApplyMappings(url);
}

void Context::OnShutdownComplete() {
  DCHECK_EQ(base::MessageLoop::current()->task_runner(),
            task_runners_->shell_runner());
  base::MessageLoop::current()->Quit();
}

void Context::Run(const GURL& url) {
  ServiceProviderPtr services;
  ServiceProviderPtr exposed_services;

  app_urls_.insert(url);
  application_manager_.ConnectToApplication(url, GURL(), GetProxy(&services),
                                            exposed_services.Pass());
}

ScopedMessagePipeHandle Context::ConnectToServiceByName(
    const GURL& application_url,
    const std::string& service_name) {
  app_urls_.insert(application_url);
  return application_manager_.ConnectToServiceByName(application_url,
                                                     service_name).Pass();
}

}  // namespace shell
}  // namespace mojo
