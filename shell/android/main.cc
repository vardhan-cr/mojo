// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/android/main.h"

#include "base/android/fifo_utils.h"
#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/simple_thread.h"
#include "jni/ShellMain_jni.h"
#include "mojo/common/message_pump_mojo.h"
#include "mojo/services/window_manager/public/interfaces/window_manager.mojom.h"
#include "shell/android/android_handler_loader.h"
#include "shell/android/java_application_loader.h"
#include "shell/android/native_viewport_application_loader.h"
#include "shell/android/ui_application_loader_android.h"
#include "shell/application_manager/application_loader.h"
#include "shell/background_application_loader.h"
#include "shell/command_line_util.h"
#include "shell/context.h"
#include "shell/crash/breakpad.h"
#include "shell/init.h"
#include "shell/switches.h"
#include "shell/tracer.h"
#include "ui/gl/gl_surface_egl.h"

using base::LazyInstance;

namespace shell {

namespace {

// Tag for logging.
const char kLogTag[] = "chromium";

// Command line argument for the communication fifo.
const char kFifoPath[] = "fifo-path";

class MojoShellRunner : public base::DelegateSimpleThread::Delegate {
 public:
  MojoShellRunner(const base::FilePath& mojo_shell_child_path,
                  const std::vector<std::string>& parameters)
      : mojo_shell_child_path_(mojo_shell_child_path),
        parameters_(parameters) {}
  ~MojoShellRunner() override {}

 private:
  void Run() override;

  const base::FilePath mojo_shell_child_path_;
  const std::vector<std::string> parameters_;

  DISALLOW_COPY_AND_ASSIGN(MojoShellRunner);
};

// Structure to hold internal data used to setup the Mojo shell.
struct InternalShellData {
  // java_message_loop is the main thread/UI thread message loop.
  scoped_ptr<base::MessageLoop> java_message_loop;

  // tracer, accessible on the main thread
  scoped_ptr<Tracer> tracer;

  // Shell context, accessible on the shell thread
  scoped_ptr<Context> context;

  // Shell runner (thread delegate), to be uesd by the shell thread.
  scoped_ptr<MojoShellRunner> shell_runner;

  // Thread to run the shell on.
  scoped_ptr<base::DelegateSimpleThread> shell_thread;

  // Main android app activity, to be used on the main thread.
  base::android::ScopedJavaGlobalRef<jobject> main_activity;

  // Proxy to a Window Manager, initialized on the shell thread.
  mojo::WindowManagerPtr window_manager;

  // TaskRunner used to execute tasks on the shell thread.
  scoped_refptr<base::SingleThreadTaskRunner> shell_task_runner;

  // Event signalling when the shell task runner has been initialized.
  scoped_ptr<base::WaitableEvent> shell_runner_ready;
};

LazyInstance<InternalShellData> g_internal_data = LAZY_INSTANCE_INITIALIZER;

void ConfigureAndroidServices(Context* context) {
  context->application_manager()->SetLoaderForURL(
      make_scoped_ptr(new UIApplicationLoader(
          make_scoped_ptr(new NativeViewportApplicationLoader()),
          g_internal_data.Get().java_message_loop.get())),
      GURL("mojo:native_viewport_service"));

  // Android handler is bundled with the Mojo shell, because it uses the
  // MojoShell application as the JNI bridge to bootstrap execution of other
  // Android Mojo apps that need JNI.
  context->application_manager()->SetLoaderForURL(
      make_scoped_ptr(new BackgroundApplicationLoader(
          make_scoped_ptr(new AndroidHandlerLoader()), "android_handler",
          base::MessageLoop::TYPE_DEFAULT)),
      GURL("mojo:android_handler"));

  // Register java applications.
  base::android::ScopedJavaGlobalRef<jobject> java_application_registry(
      JavaApplicationLoader::CreateJavaApplicationRegistry());
  for (const auto& url : JavaApplicationLoader::GetApplicationURLs(
           java_application_registry.obj())) {
    context->application_manager()->SetLoaderForURL(
        make_scoped_ptr(
            new JavaApplicationLoader(java_application_registry, url)),
        GURL(url));
  }

  // By default, the authenticated_network_service is handled by the
  // authentication service.
  context->url_resolver()->AddURLMapping(
      GURL("mojo:authenticated_network_service"), GURL("mojo:authentication"));
}

void QuitShellThread() {
  g_internal_data.Get().shell_thread->Join();
  g_internal_data.Get().shell_thread.reset();
  Java_ShellMain_finishActivity(base::android::AttachCurrentThread(),
                                g_internal_data.Get().main_activity.obj());
  exit(0);
}

void MojoShellRunner::Run() {
  base::MessageLoop loop(mojo::common::MessagePumpMojo::Create());
  g_internal_data.Get().shell_task_runner = loop.task_runner();
  // Signal that the shell is ready to receive requests.
  g_internal_data.Get().shell_runner_ready->Signal();

  Context* context = g_internal_data.Get().context.get();
  ConfigureAndroidServices(context);
  context->InitWithPaths(mojo_shell_child_path_);

  for (const auto& args : parameters_)
    ApplyApplicationArgs(context, args);

  RunCommandLineApps(context);

  loop.Run();

  g_internal_data.Get().java_message_loop.get()->PostTask(
      FROM_HERE, base::Bind(&QuitShellThread));
}

// Initialize stdout redirection if the command line switch is present.
void InitializeRedirection() {
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(kFifoPath))
    return;

  base::FilePath fifo_path =
      base::CommandLine::ForCurrentProcess()->GetSwitchValuePath(kFifoPath);
  base::FilePath directory = fifo_path.DirName();
  CHECK(base::CreateDirectoryAndGetError(directory, nullptr))
      << "Unable to create directory: " << directory.value();
  unlink(fifo_path.value().c_str());
  CHECK(base::android::CreateFIFO(fifo_path, 0666))
      << "Unable to create fifo: " << fifo_path.value();
  CHECK(base::android::RedirectStream(stdout, fifo_path, "w"))
      << "Failed to redirect stdout to file: " << fifo_path.value();
  CHECK(dup2(STDOUT_FILENO, STDERR_FILENO) != -1)
      << "Unable to redirect stderr to stdout.";
}

void EmbedApplicationByURL(std::string url) {
  if (!g_internal_data.Get().window_manager.get()) {
    Context* context = g_internal_data.Get().context.get();
    context->application_manager()->ConnectToService(
        GURL("mojo:window_manager"), &g_internal_data.Get().window_manager);
  }
  g_internal_data.Get().window_manager->Embed(url, nullptr, nullptr);
}

void ConnectToApplicationImpl(
    const GURL& url,
    mojo::ScopedMessagePipeHandle services_handle,
    mojo::ScopedMessagePipeHandle exposed_services_handle) {
  Context* context = g_internal_data.Get().context.get();
  mojo::InterfaceRequest<mojo::ServiceProvider> services;
  services.Bind(services_handle.Pass());
  mojo::ServiceProviderPtr exposed_services;
  exposed_services.Bind(mojo::InterfacePtrInfo<mojo::ServiceProvider>(
      exposed_services_handle.Pass(), 0u));
  context->application_manager()->ConnectToApplication(
      url, GURL(), services.Pass(), exposed_services.Pass(), base::Closure());
}

}  // namespace

static void Start(JNIEnv* env,
                  jclass clazz,
                  jobject activity,
                  jstring mojo_shell_child_path,
                  jobjectArray jparameters,
                  jstring j_local_apps_directory,
                  jstring j_tmp_dir,
                  jstring j_home_dir) {
  g_internal_data.Get().main_activity.Reset(env, activity);
  // Initially, the shell runner is not ready.
  g_internal_data.Get().shell_runner_ready.reset(
      new base::WaitableEvent(true, false));

  std::string tmp_dir = base::android::ConvertJavaStringToUTF8(env, j_tmp_dir);
  // Setting the TMPDIR and HOME environment variables so that applications can
  // use it.
  // TODO(qsr) We will need our subprocesses to inherit this.
  int return_value = setenv("TMPDIR", tmp_dir.c_str(), 1);
  DCHECK_EQ(return_value, 0);
  return_value = setenv(
      "HOME", base::android::ConvertJavaStringToUTF8(env, j_home_dir).c_str(),
      1);
  DCHECK_EQ(return_value, 0);

  base::android::ScopedJavaLocalRef<jobject> scoped_activity(env, activity);
  base::android::InitApplicationContext(env, scoped_activity);

  std::vector<std::string> parameters;
  parameters.push_back("mojo_shell");
  base::android::AppendJavaStringArrayToStringVector(env, jparameters,
                                                     &parameters);
  base::CommandLine::Init(0, nullptr);
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->InitFromArgv(parameters);

  base::FilePath dumps_path = base::FilePath(tmp_dir).Append("breakpad_dumps");
  breakpad::InitCrashReporter(dumps_path);

  Tracer* tracer = new Tracer;
  g_internal_data.Get().tracer.reset(tracer);
  bool trace_startup = command_line->HasSwitch(switches::kTraceStartup);
  if (trace_startup) {
    tracer->Start(
        command_line->GetSwitchValueASCII(switches::kTraceStartup),
        command_line->GetSwitchValueASCII(switches::kTraceStartupDuration),
        tmp_dir + "/mojo_shell.trace");
  }

  g_internal_data.Get().shell_runner.reset(
      new MojoShellRunner(base::FilePath(base::android::ConvertJavaStringToUTF8(
                              env, mojo_shell_child_path)),
                          parameters));

  InitializeLogging();

  InitializeRedirection();

  // We want ~MessageLoop to happen prior to ~Context. Initializing
  // LazyInstances is akin to stack-allocating objects; their destructors
  // will be invoked first-in-last-out.
  Context* shell_context = new Context(tracer);
  shell_context->SetShellFileRoot(base::FilePath(
      base::android::ConvertJavaStringToUTF8(env, j_local_apps_directory)));
  g_internal_data.Get().context.reset(shell_context);

  g_internal_data.Get().java_message_loop.reset(new base::MessageLoopForUI);
  base::MessageLoopForUI::current()->Start();
  tracer->DidCreateMessageLoop();

  g_internal_data.Get().shell_thread.reset(new base::DelegateSimpleThread(
      g_internal_data.Get().shell_runner.get(), "ShellThread"));
  g_internal_data.Get().shell_thread->Start();

  // TODO(abarth): At which point should we switch to cross-platform
  // initialization?

  gfx::GLSurface::InitializeOneOff();

  g_internal_data.Get().shell_runner_ready->Wait();
}

static void AddApplicationURL(JNIEnv* env, jclass clazz, jstring jurl) {
  base::CommandLine::ForCurrentProcess()->AppendArg(
      base::android::ConvertJavaStringToUTF8(env, jurl));
}

static void StartApplicationURL(JNIEnv* env, jclass clazz, jstring jurl) {
  std::string url = base::android::ConvertJavaStringToUTF8(env, jurl);
  g_internal_data.Get().shell_task_runner->PostTask(
      FROM_HERE, base::Bind(&EmbedApplicationByURL, url));
}

static void ConnectToApplication(JNIEnv* env,
                                 jclass clazz,
                                 jstring jurl,
                                 jint services_handle,
                                 jint exposed_services_handle) {
  GURL url = GURL(base::android::ConvertJavaStringToUTF8(env, jurl));
  g_internal_data.Get().shell_task_runner->PostTask(
      FROM_HERE,
      base::Bind(&ConnectToApplicationImpl, url,
                 base::Passed(mojo::ScopedMessagePipeHandle(
                     mojo::MessagePipeHandle(services_handle))),
                 base::Passed(mojo::ScopedMessagePipeHandle(
                     mojo::MessagePipeHandle(exposed_services_handle)))));
}

bool RegisterShellMain(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace shell
