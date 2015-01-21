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
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/simple_platform_support.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "services/tracing/tracing.mojom.h"
#include "shell/application_manager/application_loader.h"
#include "shell/application_manager/application_manager.h"
#include "shell/dynamic_application_loader.h"
#include "shell/external_application_listener.h"
#include "shell/in_process_dynamic_service_runner.h"
#include "shell/out_of_process_dynamic_service_runner.h"
#include "shell/switches.h"
#include "url/gurl.h"

namespace mojo {
namespace shell {
namespace {

// These mojo: URLs are loaded directly from the local filesystem. They
// correspond to shared libraries bundled alongside the mojo_shell.
const char* kLocalMojoURLs[] = {
    "mojo:network_service",
};

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

static base::LazyInstance<Setup>::Leaky setup = LAZY_INSTANCE_INITIALIZER;

void InitContentHandlers(DynamicApplicationLoader* loader,
                         base::CommandLine* command_line) {
  // Default content handlers.
  loader->RegisterContentHandler("application/pdf", GURL("mojo:pdf_viewer"));
  loader->RegisterContentHandler("image/png", GURL("mojo:png_viewer"));
  loader->RegisterContentHandler("text/html", GURL("mojo:html_viewer"));

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
    loader->RegisterContentHandler(parts[i], url);
  }
}

bool ConfigureURLMappings(const std::string& mappings,
                          mojo::shell::MojoURLResolver* resolver) {
  base::StringPairs pairs;
  if (!base::SplitStringIntoKeyValuePairs(mappings, '=', ',', &pairs))
    return false;
  using StringPair = std::pair<std::string, std::string>;
  for (const StringPair& pair : pairs) {
    const GURL from(pair.first);
    const GURL to(pair.second);
    if (!from.is_valid() || !to.is_valid())
      return false;
    resolver->AddCustomMapping(from, to);
  }
  return true;
}

}  // namespace

Context::Context() : application_manager_(this) {
  DCHECK(!base::MessageLoop::current());
}

Context::~Context() {
  DCHECK(!base::MessageLoop::current());
}

void Context::EnsureEmbedderIsInitialized() {
  setup.Get();
}

bool Context::Init() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  if (command_line->HasSwitch(switches::kWaitForDebugger))
    base::debug::WaitForDebugger(60, true);

  EnsureEmbedderIsInitialized();
  task_runners_.reset(
      new TaskRunners(base::MessageLoop::current()->message_loop_proxy()));

  for (size_t i = 0; i < arraysize(kLocalMojoURLs); ++i)
    mojo_url_resolver_.AddLocalFileMapping(GURL(kLocalMojoURLs[i]));

  if (command_line->HasSwitch(switches::kEnableExternalApplications)) {
    listener_ = ExternalApplicationListener::Create(
        task_runners_->shell_runner(), task_runners_->io_runner());

    base::FilePath socket_path =
        command_line->GetSwitchValuePath(switches::kEnableExternalApplications);
    if (socket_path.empty())
      socket_path = ExternalApplicationListener::ConstructDefaultSocketPath();

    listener_->ListenInBackground(
        socket_path,
        base::Bind(&ApplicationManager::RegisterExternalApplication,
                   base::Unretained(&application_manager_)));
  }
  if (command_line->HasSwitch(switches::kOrigin)) {
    mojo_url_resolver()->SetBaseURL(
        GURL(command_line->GetSwitchValueASCII(switches::kOrigin)));
  }
  if (command_line->HasSwitch(switches::kURLMappings) &&
      !ConfigureURLMappings(
          command_line->GetSwitchValueASCII(switches::kURLMappings),
          mojo_url_resolver())) {
    return false;
  }

  scoped_ptr<DynamicServiceRunnerFactory> runner_factory;
  if (command_line->HasSwitch(switches::kEnableMultiprocess))
    runner_factory.reset(new OutOfProcessDynamicServiceRunnerFactory());
  else
    runner_factory.reset(new InProcessDynamicServiceRunnerFactory());

  DynamicApplicationLoader* dynamic_application_loader =
      new DynamicApplicationLoader(this, runner_factory.Pass());
  InitContentHandlers(dynamic_application_loader, command_line);
  application_manager_.set_default_loader(
      scoped_ptr<ApplicationLoader>(dynamic_application_loader));

  tracing::TraceDataCollectorPtr trace_data_collector_ptr;
  application_manager_.ConnectToService(GURL("mojo:tracing"),
                                        &trace_data_collector_ptr);
  TracingImpl::Create(trace_data_collector_ptr.Pass());

  if (listener_)
    listener_->WaitForListening();

  return true;
}

void Context::OnApplicationError(const GURL& url) {
  if (app_urls_.find(url) != app_urls_.end()) {
    app_urls_.erase(url);
    if (app_urls_.empty() && base::MessageLoop::current()->is_running())
      base::MessageLoop::current()->Quit();
  }
}

GURL Context::ResolveURL(const GURL& url) {
  return mojo_url_resolver_.Resolve(url);
}

GURL Context::ResolveMappings(const GURL& url) {
  return mojo_url_resolver_.ApplyCustomMappings(url);
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
