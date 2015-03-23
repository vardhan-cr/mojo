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
#include "mojo/public/c/system/core.h"

namespace mojo {
namespace dart {

extern const uint8_t* snapshot_buffer;

const char* kDartScheme = "dart:";
const char* kAsyncLibURL = "dart:async";
const char* kInternalLibURL = "dart:_internal";
const char* kIsolateLibURL = "dart:isolate";
const char* kIOLibURL = "dart:io";
const char* kCoreLibURL = "dart:core";

static bool IsDartSchemeURL(const char* url_name) {
  static const intptr_t kDartSchemeLen = strlen(kDartScheme);
  // If the URL starts with "dart:" then it is considered as a special
  // library URL which is handled differently from other URLs.
  return (strncmp(url_name, kDartScheme, kDartSchemeLen) == 0);
}

static bool IsServiceIsolateURL(const char* url_name) {
  if (url_name == nullptr) {
    return false;
  }
  static const intptr_t kServiceIsolateNameLen =
      strlen(DART_VM_SERVICE_ISOLATE_NAME);
  return (strncmp(url_name,
                  DART_VM_SERVICE_ISOLATE_NAME,
                  kServiceIsolateNameLen) == 0);
}

static bool IsDartIOLibURL(const char* url_name) {
  return (strcmp(url_name, kIOLibURL) == 0);
}

static void ReportScriptError(Dart_Handle handle) {
  // The normal DART_CHECK_VALID macro displays error information and a stack
  // dump for the C++ application, which is confusing. Only show the Dart error.
  if (Dart_IsError((handle))) {
    LOG(ERROR) << "Dart runtime error:\n" << Dart_GetError(handle) << "\n";
  }
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
  const char* url_string = nullptr;
  result = Dart_StringToCString(url, &url_string);
  if (tag == Dart_kCanonicalizeUrl) {
    if (Dart_IsError(result)) {
      return result;
    }
    const bool is_internal_scheme_url = IsDartSchemeURL(url_string);
    // If this is a Dart Scheme URL, or a Mojo Scheme URL, then it is not
    // modified as it will be handled internally.
    if (is_internal_scheme_url) {
      return url;
    }
    // Resolve the url within the context of the library's URL.
    Dart_Handle builtin_lib =
        Builtin::GetLibrary(Builtin::kBuiltinLibrary);
    return ResolveUri(library_url, url, builtin_lib);
  }

  Dart_Handle builtin_lib =
      Builtin::GetLibrary(Builtin::kBuiltinLibrary);
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
  Dart_Handle url = Dart_NewStringFromCString(kInternalLibURL);
  DART_CHECK_VALID(url);
  Dart_Handle internal_lib = Dart_LookupLibrary(url);
  DART_CHECK_VALID(internal_lib);
  url = Dart_NewStringFromCString(kCoreLibURL);
  DART_CHECK_VALID(url);
  Dart_Handle core_lib = Dart_LookupLibrary(url);
  DART_CHECK_VALID(internal_lib);
  url = Dart_NewStringFromCString(kAsyncLibURL);
  DART_CHECK_VALID(url);
  Dart_Handle async_lib = Dart_LookupLibrary(url);
  DART_CHECK_VALID(async_lib);
  url = Dart_NewStringFromCString(kIsolateLibURL);
  DART_CHECK_VALID(url);
  Dart_Handle isolate_lib = Dart_LookupLibrary(url);
  DART_CHECK_VALID(isolate_lib);
  Dart_Handle mojo_internal_lib =
      Builtin::GetLibrary(Builtin::kMojoInternalLibrary);
  DART_CHECK_VALID(mojo_internal_lib);

  // We need to ensure that all the scripts loaded so far are finalized
  // as we are about to invoke some Dart code below to setup closures.
  Dart_Handle result = Dart_FinalizeLoading(false);
  DART_CHECK_VALID(result);

  // Import dart:_internal into dart:mojo.builtin for setting up hooks.
  result = Dart_LibraryImportLibrary(builtin_lib, internal_lib, Dart_Null());
  DART_CHECK_VALID(result);

  // Setup the internal library's 'internalPrint' function.
  Dart_Handle print = Dart_Invoke(builtin_lib,
                                  Dart_NewStringFromCString("_getPrintClosure"),
                                  0,
                                  nullptr);
  DART_CHECK_VALID(print);
  result = Dart_SetField(internal_lib,
                         Dart_NewStringFromCString("_printClosure"),
                         print);
  DART_CHECK_VALID(result);

  DART_CHECK_VALID(Dart_Invoke(
      builtin_lib, Dart_NewStringFromCString("_setupHooks"), 0, NULL));
  DART_CHECK_VALID(Dart_Invoke(
      isolate_lib, Dart_NewStringFromCString("_setupHooks"), 0, NULL));

  // Setup the 'scheduleImmediate' closure.
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
  result = Dart_Invoke(builtin_lib,
                     Dart_NewStringFromCString("_setPackageRoot"),
                     kNumArgs,
                     dart_args);
  DART_CHECK_VALID(result);

  // Setup the uriBase with the base uri of the mojo app.
  Dart_Handle uri_base = Dart_Invoke(
      builtin_lib,
      Dart_NewStringFromCString("_getUriBaseClosure"),
      0,
      NULL);
  DART_CHECK_VALID(uri_base);
  result = Dart_SetField(core_lib,
                         Dart_NewStringFromCString("_uriBaseClosure"),
                         uri_base);
  DART_CHECK_VALID(result);

  return result;
}

static Dart_Isolate CreateServiceIsolateHelper(const char* script_uri,
                                               char** error) {
  // TODO(johnmccutchan): Add support the service isolate.
  // No callbacks for service isolate.
  IsolateCallbacks callbacks;
  IsolateData* isolate_data =
      new IsolateData(NULL, false, callbacks, "", "", "");
  Dart_Isolate isolate = Dart_CreateIsolate(script_uri,
                                            "main",
                                            snapshot_buffer,
                                            isolate_data,
                                            error);
  if (isolate == nullptr) {
    delete isolate_data;
    return nullptr;
  }
  return isolate;
}

static Dart_Isolate CreateIsolateHelper(void* dart_app,
                                        bool strict_compilation,
                                        IsolateCallbacks callbacks,
                                        const std::string& script,
                                        const std::string& script_uri,
                                        const std::string& package_root,
                                        char** error) {
  IsolateData* isolate_data = new IsolateData(
      dart_app,
      strict_compilation,
      callbacks,
      script,
      script_uri,
      package_root);
  Dart_Isolate isolate = Dart_CreateIsolate(
      script_uri.c_str(), "main", snapshot_buffer, isolate_data, error);
  if (isolate == nullptr) {
    delete isolate_data;
    return nullptr;
  }

  DPCHECK(!Dart_IsServiceIsolate(isolate));
  Dart_EnterScope();

  Dart_IsolateSetStrictCompilation(strict_compilation);

  // Setup the native resolvers for the builtin libraries as they are not set
  // up when the snapshot is read.
  CHECK(snapshot_buffer != nullptr);
  Builtin::PrepareLibrary(Builtin::kBuiltinLibrary);
  Builtin::PrepareLibrary(Builtin::kMojoInternalLibrary);

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
      Builtin::GetLibrary(Builtin::kBuiltinLibrary);
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
  ReportScriptError(result);

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

  if (IsServiceIsolateURL(script_uri)) {
    return CreateServiceIsolateHelper(script_uri, error);
  }
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
                             parent_isolate_data->strict_compilation,
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

  // Close handles generated by the isolate.
  std::set<MojoHandle>& handles = isolate_data->unclosed_handles;
  for (auto it = handles.begin(); it != handles.end(); ++it) {
    MojoClose((*it));
  }
  handles.clear();
}


bool DartController::initialized_ = false;
Dart_Isolate DartController::root_isolate_ = nullptr;
bool DartController::strict_compilation_ = false;

void DartController::InitVmIfNeeded(Dart_EntropySource entropy,
                                    const char** arguments,
                                    int arguments_count) {
  // TODO(zra): If runDartScript can be called from multiple threads
  // concurrently, then initialized_ will need to be protected by a lock.
  if (initialized_) {
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
                           entropy);
  CHECK(result);
  initialized_ = true;
}

bool DartController::RunSingleDartScript(const DartControllerConfig& config) {
  InitVmIfNeeded(config.entropy,
                 config.arguments,
                 config.arguments_count);
  Dart_Isolate isolate = CreateIsolateHelper(config.application_data,
                                             config.strict_compilation,
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

  // Start the MojoHandleWatcher.
  Dart_Handle mojo_internal_lib =
      Builtin::GetLibrary(Builtin::kMojoInternalLibrary);
  DART_CHECK_VALID(mojo_internal_lib);
  Dart_Handle handle_watcher_type =
      Dart_GetType(mojo_internal_lib,
                   Dart_NewStringFromCString("MojoHandleWatcher"), 0, nullptr);
  DART_CHECK_VALID(handle_watcher_type);
  result = Dart_Invoke(
      handle_watcher_type,
      Dart_NewStringFromCString("_start"),
      0,
      nullptr);
  DART_CHECK_VALID(result);

  // RunLoop until the handle watcher isolate is spun-up.
  result = Dart_RunLoop();
  DART_CHECK_VALID(result);

  // Load the root library into the builtin library so that main can be found.
  Dart_Handle builtin_lib =
      Builtin::GetLibrary(Builtin::kBuiltinLibrary);
  DART_CHECK_VALID(builtin_lib);
  Dart_Handle root_lib = Dart_RootLibrary();
  DART_CHECK_VALID(root_lib);
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
  isolate_args[1] = Dart_NewList(2);  // args
  DART_CHECK_VALID(isolate_args[1]);

  Dart_Handle script_uri = Dart_NewStringFromUTF8(
      reinterpret_cast<const uint8_t*>(config.script_uri.data()),
      config.script_uri.length());
  Dart_ListSetAt(isolate_args[1], 0, Dart_NewInteger(config.handle));
  Dart_ListSetAt(isolate_args[1], 1, script_uri);

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
      Dart_NewStringFromCString("_stop"),
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

static bool generateEntropy(uint8_t* buffer, intptr_t length) {
  crypto::RandBytes(reinterpret_cast<void*>(buffer), length);
  return true;
}

bool DartController::Initialize(bool strict_compilation) {
  char* error;
  strict_compilation_ = strict_compilation;

  // No callbacks for root isolate.
  IsolateCallbacks callbacks;
  InitVmIfNeeded(generateEntropy, nullptr, 0);
  root_isolate_ = CreateIsolateHelper(
      nullptr, false, callbacks, "", "", "", &error);
  if (root_isolate_ == nullptr) {
    LOG(ERROR) << error;
    Dart_Cleanup();
    initialized_ = false;
    return false;
  }

  Dart_EnterIsolate(root_isolate_);
  Dart_Handle result;
  Dart_EnterScope();

  // Start the MojoHandleWatcher.
  Dart_Handle mojo_internal_lib =
      Builtin::GetLibrary(Builtin::kMojoInternalLibrary);
  DART_CHECK_VALID(mojo_internal_lib);
  Dart_Handle handle_watcher_type =
      Dart_GetType(mojo_internal_lib,
                   Dart_NewStringFromCString("MojoHandleWatcher"), 0, nullptr);
  DART_CHECK_VALID(handle_watcher_type);
  result = Dart_Invoke(
      handle_watcher_type,
      Dart_NewStringFromCString("_start"),
      0,
      nullptr);
  DART_CHECK_VALID(result);

  // RunLoop until the handle watcher isolate is spun-up.
  result = Dart_RunLoop();
  DART_CHECK_VALID(result);

  Dart_ExitScope();
  Dart_ExitIsolate();
  return true;
}

bool DartController::RunDartScript(const DartControllerConfig& config) {
  CHECK(root_isolate_ != nullptr);
  const bool strict = strict_compilation_ || config.strict_compilation;
  Dart_Isolate isolate = CreateIsolateHelper(
      config.application_data, strict, config.callbacks, config.script,
      config.script_uri, config.package_root, config.error);
  if (isolate == nullptr) {
    return false;
  }

  Dart_EnterIsolate(isolate);
  Dart_Handle result;
  Dart_EnterScope();

  // Load the root library into the builtin library so that main can be found.
  Dart_Handle builtin_lib =
      Builtin::GetLibrary(Builtin::kBuiltinLibrary);
  DART_CHECK_VALID(builtin_lib);
  Dart_Handle root_lib = Dart_RootLibrary();
  DART_CHECK_VALID(root_lib);
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
  isolate_args[1] = Dart_NewList(2);  // args
  DART_CHECK_VALID(isolate_args[1]);

  Dart_Handle script_uri = Dart_NewStringFromUTF8(
      reinterpret_cast<const uint8_t*>(config.script_uri.data()),
      config.script_uri.length());
  Dart_ListSetAt(isolate_args[1], 0, Dart_NewInteger(config.handle));
  Dart_ListSetAt(isolate_args[1], 1, script_uri);

  Dart_Handle isolate_lib =
      Dart_LookupLibrary(Dart_NewStringFromCString(kIsolateLibURL));
  DART_CHECK_VALID(isolate_lib);

  result = Dart_Invoke(isolate_lib,
                       Dart_NewStringFromCString("_startMainIsolate"),
                       kNumIsolateArgs,
                       isolate_args);
  DART_CHECK_VALID(result);

  // Run main until completion.
  result = Dart_RunLoop();
  ReportScriptError(result);

  Dart_ExitScope();
  Dart_ShutdownIsolate();
  return true;
}

void DartController::Shutdown() {
  CHECK(root_isolate_ != nullptr);
  Dart_EnterIsolate(root_isolate_);
  Dart_Handle result;
  Dart_EnterScope();

  // Stop the MojoHandleWatcher.
  Dart_Handle mojo_internal_lib =
      Builtin::GetLibrary(Builtin::kMojoInternalLibrary);
  DART_CHECK_VALID(mojo_internal_lib);
  Dart_Handle handle_watcher_type =
      Dart_GetType(mojo_internal_lib,
                   Dart_NewStringFromCString("MojoHandleWatcher"), 0, nullptr);
  DART_CHECK_VALID(handle_watcher_type);
  result = Dart_Invoke(
      handle_watcher_type,
      Dart_NewStringFromCString("_stop"),
      0,
      nullptr);
  DART_CHECK_VALID(result);

  result = Dart_RunLoop();
  DART_CHECK_VALID(result);

  Dart_ExitScope();
  Dart_ShutdownIsolate();
  Dart_Cleanup();
  root_isolate_ = nullptr;
  initialized_ = false;
}

}  // namespace apps
}  // namespace mojo
