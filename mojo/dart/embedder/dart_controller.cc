// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/sys_info.h"
#include "crypto/random.h"
#include "dart/runtime/include/dart_api.h"
#include "mojo/dart/embedder/builtin.h"
#include "mojo/dart/embedder/dart_controller.h"
#include "mojo/dart/embedder/isolate_data.h"

namespace mojo {
namespace dart {

// snapshot_buffer points to a snapshot if we link in a snapshot otherwise
// it is initialized to NULL.
extern const uint8_t* snapshot_buffer;

const char* kDartScheme = "dart:";
const char* kAsyncLibURL = "dart:async";
const char* kBuiltinLibURL = "dart:_builtin";
const char* kInternalLibURL = "dart:_internal";
const char* kIsolateLibURL = "dart:isolate";
const char* kIOLibURL = "dart:io";

static Dart_Handle SetWorkingDirectory(Dart_Handle builtin_lib) {
  base::FilePath current_dir;
  PathService::Get(base::DIR_CURRENT, &current_dir);

  const int kNumArgs = 1;
  Dart_Handle dart_args[kNumArgs];
  dart_args[0] = Dart_NewStringFromUTF8(
      reinterpret_cast<const uint8_t*>(current_dir.AsUTF8Unsafe().c_str()),
      current_dir.AsUTF8Unsafe().length());
  return Dart_Invoke(builtin_lib,
                     Dart_NewStringFromCString("_setWorkingDirectory"),
                     kNumArgs,
                     dart_args);
}


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
                                        Dart_Handle builtin_lib) {
  const int kNumArgs = 3;
  Dart_Handle dart_args[kNumArgs];
  dart_args[0] = tag;
  dart_args[1] = url;
  dart_args[2] = library_url;
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
  const char* library_url_string = NULL;
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
    const char* url_string = NULL;
    result = Dart_StringToCString(url, &url_string);
    if (Dart_IsError(result)) {
      return result;
    }
    bool is_dart_scheme_url = IsDartSchemeURL(url_string);
    // If this is a Dart Scheme URL or 'part' of a io library
    // then it is not modified as it will be handled internally.
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
  return LoadDataAsync_Invoke(Dart_NewInteger(tag), url, library_url,
                              builtin_lib);
}


static Dart_Handle PrepareScriptForLoading(const std::string& package_root,
                                           Dart_Handle builtin_lib) {
  // First ensure all required libraries are available.
  Dart_Handle url = Dart_NewStringFromCString(kAsyncLibURL);
  DART_CHECK_VALID(url);
  Dart_Handle async_lib = Dart_LookupLibrary(url);
  DART_CHECK_VALID(async_lib);

  // Setup the internal library's 'internalPrint' function.
  Dart_Handle print = Dart_Invoke(builtin_lib,
                                  Dart_NewStringFromCString("_getPrintClosure"),
                                  0,
                                  NULL);
  url = Dart_NewStringFromCString(kInternalLibURL);
  DART_CHECK_VALID(url);
  Dart_Handle internal_lib = Dart_LookupLibrary(url);
  DART_CHECK_VALID(internal_lib);
  Dart_Handle result = Dart_SetField(internal_lib,
                                     Dart_NewStringFromCString("_printClosure"),
                                     print);
  DART_CHECK_VALID(result);

  // TODO(zra): Set up timer factory closure. This must be implemented through
  // the mojo handle watcher just as timers are implemented in the standalone
  // embedder with epoll.

  // Setup the 'scheduleImmediate' closure.
  url = Dart_NewStringFromCString(kIsolateLibURL);
  DART_CHECK_VALID(url);
  Dart_Handle isolate_lib = Dart_LookupLibrary(url);
  DART_CHECK_VALID(isolate_lib);
  Dart_Handle schedule_immediate_closure = Dart_Invoke(
      isolate_lib,
      Dart_NewStringFromCString("_getIsolateScheduleImmediateClosure"),
      0,
      NULL);
  Dart_Handle args[1];
  args[0] = schedule_immediate_closure;
  result = Dart_Invoke(
      async_lib,
      Dart_NewStringFromCString("_setScheduleImmediateClosure"),
      1,
      args);
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


static Dart_Isolate createIsolateHelper(void* dart_app,
                                        Dart_IsolateCreateCallback app_callback,
                                        const std::string& script,
                                        const std::string& script_uri,
                                        const std::string& package_root,
                                        char** error) {
  IsolateData* isolate_data =
      new IsolateData(dart_app, app_callback, script, script_uri, package_root);
  Dart_Isolate isolate = Dart_CreateIsolate(
      script_uri.c_str(), "main", snapshot_buffer, isolate_data, error);
  if (isolate == NULL) {
    delete isolate_data;
    return NULL;
  }

  Dart_EnterScope();

  if (app_callback) {
    app_callback(script_uri.c_str(),
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

  // We need to ensure that all the scripts loaded so far are finalized
  // as we are about to invoke some Dart code below to setup closures.
  result = Dart_FinalizeLoading(false);
  DART_CHECK_VALID(result);

  result = PrepareScriptForLoading(package_root, builtin_lib);
  DART_CHECK_VALID(result);

  Dart_Handle uri = Dart_NewStringFromUTF8(
      reinterpret_cast<const uint8_t*>(script_uri.c_str()),
      script_uri.length());
  DART_CHECK_VALID(uri);

  Dart_Handle script_source = Dart_NewStringFromUTF8(
      reinterpret_cast<const uint8_t*>(script.c_str()),
      script.length());
  DART_CHECK_VALID(script_source);

  Dart_Handle load_result = Dart_LoadScript(uri, script_source, 0, 0);
  DART_CHECK_VALID(load_result);

  // Make the isolate runnable so that it is ready to handle messages.
  Dart_ExitScope();
  Dart_ExitIsolate();
  bool retval = Dart_IsolateMakeRunnable(isolate);
  if (!retval) {
    *error = strdup("Invalid isolate state - Unable to make it runnable");
    Dart_EnterIsolate(isolate);
    Dart_ShutdownIsolate();
    return NULL;
  }

  return isolate;
}


static Dart_Isolate isolateCreateCallback(const char* script_uri,
                                          const char* main,
                                          const char* package_root,
                                          void* callback_data,
                                          char** error) {
  IsolateData* parent_isolate_data =
      reinterpret_cast<IsolateData*>(callback_data);
  std::string script_uri_string;
  std::string package_root_string;

  if (script_uri == NULL) {
    if (callback_data == NULL) {
      *error = strdup("Invalid 'callback_data' - Unable to spawn new isolate");
      return NULL;
    }
    script_uri_string = parent_isolate_data->script_uri;
  } else {
    script_uri_string = std::string(script_uri);
  }
  if (package_root == NULL) {
    package_root_string = parent_isolate_data->package_root;
  } else {
    package_root_string = std::string(package_root);
  }
  return createIsolateHelper(parent_isolate_data->app,
                             parent_isolate_data->app_callback,
                             parent_isolate_data->script,
                             script_uri_string,
                             package_root_string,
                             error);
}


static Dart_Isolate serviceIsolateCreateCallback(void* callback_data,
                                                 char** error) {
  if (error != NULL) {
    *error = strdup("There should be no service isolate");
  }
  return NULL;
}


bool DartController::vmIsInitialized = false;
void DartController::initVmIfNeeded(Dart_IsolateShutdownCallback shutdown,
                                    Dart_EntropySource entropy) {
  // TODO(zra): If runDartScript can be called from multiple threads
  // concurrently, then vmIsInitialized will need to be protected by a lock.
  if (vmIsInitialized) {
    return;
  }

  // TODO(zra): Take flags from the caller for testing.

  const int kNumArgs = 2;
  const char* args[kNumArgs];

  // TODO(zra): Fix Dart VM Shutdown race.
  // There is a bug in Dart VM shutdown which causes its thread pool threads
  // to potentially fail to exit when the rest of the VM is going down. This
  // results in a segfault if they begin running again after the Dart
  // embedder has been unloaded. Setting this flag to 0 ensures that these
  // threads sleep forever instead of waking up and trying to run code
  // that isn't there anymore.
  args[0] = "--worker-timeout-millis=0";

  // Enable async/await features.
  args[1] = "--enable-async";

  bool result = Dart_SetVMFlags(kNumArgs, args);
  CHECK(result);

  // TODO(zra): Provide unhandled exception callback.
  result = Dart_Initialize(isolateCreateCallback,
                           NULL,  // Isolate interrupt callback.
                           NULL,  // Unhandled exception callback.
                           shutdown,
                           // File IO callbacks.
                           NULL, NULL, NULL, NULL,
                           entropy,
                           serviceIsolateCreateCallback);
  CHECK(result);
  vmIsInitialized = true;
}


bool DartController::runDartScript(void* dart_app,
                                   const std::string& script,
                                   const std::string& script_uri,
                                   const std::string& package_root,
                                   Dart_IsolateCreateCallback app_callback,
                                   Dart_IsolateShutdownCallback shutdown,
                                   Dart_EntropySource entropy,
                                   char** error) {
  initVmIfNeeded(shutdown, entropy);
  Dart_Isolate isolate = createIsolateHelper(
      dart_app, app_callback, script, script_uri, package_root, error);
  if (isolate == NULL) {
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

  result = Dart_LibraryImportLibrary(builtin_lib, root_lib, Dart_Null());
  DART_CHECK_VALID(builtin_lib);

  Dart_Handle main_closure = Dart_Invoke(
      builtin_lib,
      Dart_NewStringFromCString("_getMainClosure"),
      0,
      NULL);
  DART_CHECK_VALID(main_closure);

  // Call _startIsolate in the isolate library to enable dispatching the
  // initial startup message.
  const intptr_t kNumIsolateArgs = 2;
  Dart_Handle isolate_args[kNumIsolateArgs];
  isolate_args[0] = main_closure;     // entryPoint
  isolate_args[1] = Dart_NewList(0);  // args
  DART_CHECK_VALID(isolate_args[1]);

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

  Dart_ExitScope();
  Dart_ShutdownIsolate();
  Dart_Cleanup();
  return true;
}

}  // namespace apps
}  // namespace mojo
