// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/callback.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/sys_info.h"
#include "crypto/random.h"
#include "dart/runtime/include/dart_api.h"
#include "dart/runtime/include/dart_native_api.h"
#include "mojo/dart/embedder/builtin.h"
#include "mojo/dart/embedder/dart_controller.h"
#include "mojo/dart/embedder/isolate_data.h"

namespace mojo {
namespace dart {

extern const uint8_t* snapshot_buffer;

const char* kDartScheme = "dart:";
const char* kAsyncLibURL = "dart:async";
const char* kInternalLibURL = "dart:_internal";
const char* kIsolateLibURL = "dart:isolate";
const char* kIOLibURL = "dart:io";

static bool IsDartSchemeURL(const char* url_name) {
  static const intptr_t kDartSchemeLen = strlen(kDartScheme);
  // If the URL starts with "dart:" then it is considered as a special
  // library URL which is handled differently from other URLs.
  return (strncmp(url_name, kDartScheme, kDartSchemeLen) == 0);
}

static bool IsDartIOLibURL(const char* url_name) {
  return (strcmp(url_name, kIOLibURL) == 0);
}

Dart_Handle ResolveUri(Dart_Handle library_url,
                       Dart_Handle url,
                       Dart_Handle builtin_lib) {
  const int kNumArgs = 2;
  Dart_Handle dart_args[kNumArgs];
  dart_args[0] = library_url;
  dart_args[1] = url;
  return Dart_Invoke(builtin_lib,
                     Dart_NewStringFromCString("_resolveUri"),
                     kNumArgs,
                     dart_args);
}

static Dart_Handle LoadDataAsync_Invoke(Dart_Handle tag,
                                        Dart_Handle url,
                                        Dart_Handle library_url,
                                        Dart_Handle builtin_lib,
                                        Dart_Handle data) {
  const int kNumArgs = 4;
  Dart_Handle dart_args[kNumArgs];
  dart_args[0] = tag;
  dart_args[1] = url;
  dart_args[2] = library_url;
  dart_args[3] = data;
  return Dart_Invoke(builtin_lib,
                     Dart_NewStringFromCString("_loadDataAsync"),
                     kNumArgs,
                     dart_args);
}

static Dart_Handle LibraryTagHandler(Dart_LibraryTag tag,
                                     Dart_Handle library,
                                     Dart_Handle url) {
  if (!Dart_IsLibrary(library)) {
    return Dart_NewApiError("not a library");
  }
  if (!Dart_IsString(url)) {
    return Dart_NewApiError("url is not a string");
  }
  Dart_Handle library_url = Dart_LibraryUrl(library);
  const char* library_url_string = nullptr;
  Dart_Handle result = Dart_StringToCString(library_url, &library_url_string);
  if (Dart_IsError(result)) {
    return result;
  }

  bool is_io_library = IsDartIOLibURL(library_url_string);

  // Handle IO library.
  if (is_io_library) {
    return Builtin::NewError(
        "The built-in io library is not available in this embedding: %s.\n",
        library_url_string);
  }

  // Handle URI canonicalization requests.
  if (tag == Dart_kCanonicalizeUrl) {
    const char* url_string = nullptr;
    result = Dart_StringToCString(url, &url_string);
    if (Dart_IsError(result)) {
      return result;
    }
    bool is_dart_scheme_url = IsDartSchemeURL(url_string);
    // If this is a Dart Scheme URL then it is not modified as it will be
    // handled internally.
    if (is_dart_scheme_url) {
      return url;
    }
    // Resolve the url within the context of the library's URL.
    Dart_Handle builtin_lib =
        Builtin::LoadAndCheckLibrary(Builtin::kBuiltinLibrary);
    return ResolveUri(library_url, url, builtin_lib);
  }

  Dart_Handle builtin_lib =
      Builtin::LoadAndCheckLibrary(Builtin::kBuiltinLibrary);
  // Handle 'import' or 'part' requests for all other URIs. Call dart code to
  // read the source code asynchronously.
  return LoadDataAsync_Invoke(
      Dart_NewInteger(tag), url, library_url, builtin_lib, Dart_Null());
}

static Dart_Handle SetWorkingDirectory(Dart_Handle builtin_lib) {
  base::FilePath current_dir;
  PathService::Get(base::DIR_CURRENT, &current_dir);

  const int kNumArgs = 1;
  Dart_Handle dart_args[kNumArgs];
  std::string current_dir_string = current_dir.AsUTF8Unsafe();
  dart_args[0] = Dart_NewStringFromUTF8(
      reinterpret_cast<const uint8_t*>(current_dir_string.data()),
      current_dir_string.length());
  return Dart_Invoke(builtin_lib,
                     Dart_NewStringFromCString("_setWorkingDirectory"),
                     kNumArgs,
                     dart_args);
}

static Dart_Handle PrepareScriptForLoading(const std::string& package_root,
                                           Dart_Handle builtin_lib) {
  // First ensure all required libraries are available.
  Dart_Handle url = Dart_NewStringFromCString(kAsyncLibURL);
  DART_CHECK_VALID(url);
  Dart_Handle async_lib = Dart_LookupLibrary(url);
  DART_CHECK_VALID(async_lib);
  Dart_Handle mojo_core_lib =
      Builtin::LoadAndCheckLibrary(Builtin::kMojoCoreLibrary);
  DART_CHECK_VALID(mojo_core_lib);

  // We need to ensure that all the scripts loaded so far are finalized
  // as we are about to invoke some Dart code below to setup closures.
  Dart_Handle result = Dart_FinalizeLoading(false);
  DART_CHECK_VALID(result);

  // Setup the internal library's 'internalPrint' function.
  Dart_Handle print = Dart_Invoke(builtin_lib,
                                  Dart_NewStringFromCString("_getPrintClosure"),
                                  0,
                                  nullptr);
  DART_CHECK_VALID(print);
  url = Dart_NewStringFromCString(kInternalLibURL);
  DART_CHECK_VALID(url);
  Dart_Handle internal_lib = Dart_LookupLibrary(url);
  DART_CHECK_VALID(internal_lib);
  result = Dart_SetField(internal_lib,
                         Dart_NewStringFromCString("_printClosure"),
                         print);
  DART_CHECK_VALID(result);

  // Setup the 'timer' factory.
  Dart_Handle timer_closure = Dart_Invoke(
      mojo_core_lib,
      Dart_NewStringFromCString("_getTimerFactoryClosure"),
      0,
      nullptr);
  DART_CHECK_VALID(timer_closure);
  Dart_Handle timer_args[1];
  timer_args[0] = timer_closure;
  result = Dart_Invoke(async_lib,
                       Dart_NewStringFromCString("_setTimerFactoryClosure"),
                       1,
                       timer_args);
  DART_CHECK_VALID(result);

  // Setup the 'scheduleImmediate' closure.
  url = Dart_NewStringFromCString(kIsolateLibURL);
  DART_CHECK_VALID(url);
  Dart_Handle isolate_lib = Dart_LookupLibrary(url);
  DART_CHECK_VALID(isolate_lib);
  Dart_Handle schedule_immediate_closure = Dart_Invoke(
      isolate_lib,
      Dart_NewStringFromCString("_getIsolateScheduleImmediateClosure"),
      0,
      nullptr);
  Dart_Handle schedule_args[1];
  schedule_args[0] = schedule_immediate_closure;
  result = Dart_Invoke(
      async_lib,
      Dart_NewStringFromCString("_setScheduleImmediateClosure"),
      1,
      schedule_args);
  DART_CHECK_VALID(result);

  // TODO(zra): Setup the uriBase with the base uri of the mojo app.

  // Set current working directory.
  result = SetWorkingDirectory(builtin_lib);
  if (Dart_IsError(result)) {
    return result;
  }

  // Set up package root.
  result = Dart_NewStringFromUTF8(
      reinterpret_cast<const uint8_t*>(package_root.c_str()),
      package_root.length());
  DART_CHECK_VALID(result);

  const int kNumArgs = 1;
  Dart_Handle dart_args[kNumArgs];
  dart_args[0] = result;
  return Dart_Invoke(builtin_lib,
                     Dart_NewStringFromCString("_setPackageRoot"),
                     kNumArgs,
                     dart_args);

  return result;
}

static Dart_Isolate CreateIsolateHelper(void* dart_app,
                                        IsolateCallbacks callbacks,
                                        const std::string& script,
                                        const std::string& script_uri,
                                        const std::string& package_root,
                                        char** error) {
  IsolateData* isolate_data =
      new IsolateData(dart_app, callbacks, script, script_uri, package_root);
  Dart_Isolate isolate = Dart_CreateIsolate(
      script_uri.c_str(), "main", snapshot_buffer, isolate_data, error);
  if (isolate == nullptr) {
    delete isolate_data;
    return nullptr;
  }

  Dart_EnterScope();

  // Setup the native resolvers for the builtin libraries as they are not set
  // up when the snapshot is read.
  CHECK(snapshot_buffer != nullptr);
  Builtin::SetNativeResolver(Builtin::kBuiltinLibrary);
  Builtin::SetNativeResolver(Builtin::kMojoCoreLibrary);

  if (!callbacks.create.is_null()) {
    callbacks.create.Run(script_uri.c_str(),
                         "main",
                         package_root.c_str(),
                         isolate_data,
                         error);
  }

  // Set up the library tag handler for this isolate.
  Dart_Handle result = Dart_SetLibraryTagHandler(LibraryTagHandler);
  DART_CHECK_VALID(result);

  // Prepare builtin and its dependent libraries for use to resolve URIs.
  // The builtin library is part of the snapshot and is already available.
  Dart_Handle builtin_lib =
      Builtin::LoadAndCheckLibrary(Builtin::kBuiltinLibrary);
  DART_CHECK_VALID(builtin_lib);

  result = PrepareScriptForLoading(package_root, builtin_lib);
  DART_CHECK_VALID(result);

  Dart_Handle uri = Dart_NewStringFromUTF8(
      reinterpret_cast<const uint8_t*>(script_uri.c_str()),
      script_uri.length());
  DART_CHECK_VALID(uri);

  const void* data = static_cast<const void*>(script.data());
  Dart_Handle script_source = Dart_NewExternalTypedData(
      Dart_TypedData_kUint8,
      const_cast<void*>(data),
      script.length());
  DART_CHECK_VALID(script_source);

  result = LoadDataAsync_Invoke(
      Dart_Null(), uri, Dart_Null(), builtin_lib, script_source);
  DART_CHECK_VALID(result);

  // Run event-loop and wait for script loading to complete.
  result = Dart_RunLoop();
  DART_CHECK_VALID(result);

  // Make the isolate runnable so that it is ready to handle messages.
  Dart_ExitScope();
  Dart_ExitIsolate();
  bool retval = Dart_IsolateMakeRunnable(isolate);
  if (!retval) {
    *error = strdup("Invalid isolate state - Unable to make it runnable");
    Dart_EnterIsolate(isolate);
    Dart_ShutdownIsolate();
    return nullptr;
  }

  return isolate;
}

static Dart_Isolate IsolateCreateCallback(const char* script_uri,
                                          const char* main,
                                          const char* package_root,
                                          void* callback_data,
                                          char** error) {
  IsolateData* parent_isolate_data =
      reinterpret_cast<IsolateData*>(callback_data);
  std::string script_uri_string;
  std::string package_root_string;

  if (script_uri == nullptr) {
    if (callback_data == nullptr) {
      *error = strdup("Invalid 'callback_data' - Unable to spawn new isolate");
      return nullptr;
    }
    script_uri_string = parent_isolate_data->script_uri;
  } else {
    script_uri_string = std::string(script_uri);
  }
  if (package_root == nullptr) {
    package_root_string = parent_isolate_data->package_root;
  } else {
    package_root_string = std::string(package_root);
  }
  return CreateIsolateHelper(parent_isolate_data->app,
                             parent_isolate_data->callbacks,
                             parent_isolate_data->script,
                             script_uri_string,
                             package_root_string,
                             error);
}

static void IsolateShutdownCallback(void* callback_data) {
  IsolateData* isolate_data = reinterpret_cast<IsolateData*>(callback_data);
  if (!isolate_data->callbacks.shutdown.is_null()) {
    isolate_data->callbacks.shutdown.Run(callback_data);
  }
  delete isolate_data;
}

static void UnhandledExceptionCallback(Dart_Handle error) {
  Dart_Isolate isolate = Dart_CurrentIsolate();
  void* data = Dart_IsolateData(isolate);
  IsolateData* isolate_data = reinterpret_cast<IsolateData*>(data);
  if (!isolate_data->callbacks.exception.is_null()) {
    // TODO(zra): Instead of passing an error handle, it may make life easier
    // for clients if we pass any error string here instead.
    isolate_data->callbacks.exception.Run(error);
  }
}

static Dart_Isolate ServiceIsolateCreateCallback(void* callback_data,
                                                 char** error) {
  if (error != nullptr) {
    *error = strdup("There should be no service isolate");
  }
  return nullptr;
}

bool DartController::vmIsInitialized = false;
void DartController::InitVmIfNeeded(Dart_EntropySource entropy,
                                    const char** arguments,
                                    int arguments_count) {
  // TODO(zra): If runDartScript can be called from multiple threads
  // concurrently, then vmIsInitialized will need to be protected by a lock.
  if (vmIsInitialized) {
    return;
  }

  const int kNumArgs = arguments_count + 1;
  const char* args[kNumArgs];

  // TODO(zra): Fix Dart VM Shutdown race.
  // There is a bug in Dart VM shutdown which causes its thread pool threads
  // to potentially fail to exit when the rest of the VM is going down. This
  // results in a segfault if they begin running again after the Dart
  // embedder has been unloaded. Setting this flag to 0 ensures that these
  // threads sleep forever instead of waking up and trying to run code
  // that isn't there anymore.
  args[0] = "--worker-timeout-millis=0";

  for (int i = 0; i < arguments_count; ++i) {
    args[i + 1] = arguments[i];
  }

  bool result = Dart_SetVMFlags(kNumArgs, args);
  CHECK(result);

  result = Dart_Initialize(IsolateCreateCallback,
                           nullptr,  // Isolate interrupt callback.
                           UnhandledExceptionCallback,
                           IsolateShutdownCallback,
                           // File IO callbacks.
                           nullptr, nullptr, nullptr, nullptr,
                           entropy,
                           ServiceIsolateCreateCallback);
  CHECK(result);
  vmIsInitialized = true;
}

bool DartController::RunDartScript(const DartControllerConfig& config) {
  InitVmIfNeeded(config.entropy,
                 config.arguments,
                 config.arguments_count);
  Dart_Isolate isolate = CreateIsolateHelper(config.application_data,
                                             config.callbacks,
                                             config.script,
                                             config.script_uri,
                                             config.package_root,
                                             config.error);
  if (isolate == nullptr) {
    return false;
  }

  Dart_EnterIsolate(isolate);
  Dart_Handle result;
  Dart_EnterScope();

  Dart_Handle root_lib = Dart_RootLibrary();
  DART_CHECK_VALID(root_lib);

  Dart_Handle builtin_lib =
      Builtin::LoadAndCheckLibrary(Builtin::kBuiltinLibrary);
  DART_CHECK_VALID(builtin_lib);
  Dart_Handle mojo_core_lib =
      Builtin::LoadAndCheckLibrary(Builtin::kMojoCoreLibrary);
  DART_CHECK_VALID(mojo_core_lib);

  // Start the MojoHandleWatcher.
  Dart_Handle handle_watcher_type = Dart_GetType(
      mojo_core_lib,
      Dart_NewStringFromCString("MojoHandleWatcher"),
      0,
      nullptr);
  DART_CHECK_VALID(handle_watcher_type);
  result = Dart_Invoke(
      handle_watcher_type,
      Dart_NewStringFromCString("Start"),
      0,
      nullptr);
  DART_CHECK_VALID(result);

  // RunLoop until the handle watcher isolate is spun-up.
  result = Dart_RunLoop();
  DART_CHECK_VALID(result);

  result = Dart_LibraryImportLibrary(builtin_lib, root_lib, Dart_Null());
  DART_CHECK_VALID(result);

  if (config.compile_all) {
    result = Dart_CompileAll();
    DART_CHECK_VALID(result);
  }

  Dart_Handle main_closure = Dart_Invoke(
      builtin_lib,
      Dart_NewStringFromCString("_getMainClosure"),
      0,
      nullptr);
  DART_CHECK_VALID(main_closure);

  // Call _startIsolate in the isolate library to enable dispatching the
  // initial startup message.
  const intptr_t kNumIsolateArgs = 2;
  Dart_Handle isolate_args[kNumIsolateArgs];
  isolate_args[0] = main_closure;     // entryPoint
  isolate_args[1] = Dart_NewList(1);  // args
  DART_CHECK_VALID(isolate_args[1]);
  Dart_ListSetAt(isolate_args[1], 0, Dart_NewInteger(config.handle));

  Dart_Handle isolate_lib =
      Dart_LookupLibrary(Dart_NewStringFromCString(kIsolateLibURL));
  DART_CHECK_VALID(isolate_lib);

  result = Dart_Invoke(isolate_lib,
                       Dart_NewStringFromCString("_startMainIsolate"),
                       kNumIsolateArgs,
                       isolate_args);
  DART_CHECK_VALID(result);

  result = Dart_RunLoop();
  DART_CHECK_VALID(result);

  // Stop the MojoHandleWatcher.
  result = Dart_Invoke(
      handle_watcher_type,
      Dart_NewStringFromCString("Stop"),
      0,
      nullptr);
  DART_CHECK_VALID(result);

  result = Dart_RunLoop();
  DART_CHECK_VALID(result);

  Dart_ExitScope();
  Dart_ShutdownIsolate();
  Dart_Cleanup();
  return true;
}

}  // namespace apps
}  // namespace mojo
