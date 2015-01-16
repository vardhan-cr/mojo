// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <python2.7/Python.h>
#include <dlfcn.h>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/i18n/icu_util.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/common/common_type_converters.h"
#include "mojo/common/data_pipe_utils.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/interface_factory_impl.h"
#include "mojo/public/python/src/common.h"
#include "third_party/zlib/google/zip_reader.h"

char kMojoSystem[] = "mojo_system";
char kMojoSystemImpl[] = "mojo_system_impl";
char kMojoMain[] = "MojoMain";

extern "C" {
  void initmojo_system();
  void initmojo_system_impl();
}

namespace mojo {
namespace python {

class PythonContentHandler : public ApplicationDelegate,
                             public ContentHandlerFactory::Delegate {
 public:
  PythonContentHandler() : content_handler_factory_(this) {}

 private:
  // Overridden from ApplicationDelegate:
  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    connection->AddService(&content_handler_factory_);
    return true;
  }

  // Extracts the target application into a temporary directory. This directory
  // is deleted at the end of the life of the PythonContentHandler object.
  std::unique_ptr<base::ScopedTempDir> ExtractApplication(
      URLResponsePtr response) {
    std::unique_ptr<base::ScopedTempDir> temp_dir(new base::ScopedTempDir);
    CHECK(temp_dir->CreateUniqueTempDir());

    zip::ZipReader reader;
    const std::string input_data = CopyToString(response->body.Pass());
    CHECK(reader.OpenFromString(input_data));
    base::FilePath temp_dir_path = temp_dir->path();
    while (reader.HasMore()) {
      CHECK(reader.OpenCurrentEntryInZip());
      CHECK(reader.ExtractCurrentEntryIntoDirectory(temp_dir_path));
      CHECK(reader.AdvanceToNextEntry());
    }
    return temp_dir;
  }

  // Sets up the Python interpreter and loads mojo system modules. This method
  // returns the global dictionary for the python environment that should be
  // used for subsequent calls. Takes as input the path of the unpacked
  // application files.
  PyObject* SetupPythonEnvironment(const std::string& application_path) {
    // TODO(etiennej): Build python ourselves so we don't have to rely on
    // dynamically loading a system library.
    dlopen("libpython2.7.so", RTLD_LAZY | RTLD_GLOBAL);

    PyImport_AppendInittab(kMojoSystemImpl, &initmojo_system_impl);
    PyImport_AppendInittab(kMojoSystem, &initmojo_system);

    Py_Initialize();

    PyObject *m, *d;
    m = PyImport_AddModule("__main__");
    if (m == NULL)
        return NULL;
    d = PyModule_GetDict(m);

    // Inject the application path into the python search path so that imports
    // from the application work as expected.
    std::string search_path_py_command =
        "import sys; sys.path.append('" + application_path + "');";
    ScopedPyRef result(
        PyRun_String(search_path_py_command.c_str(), Py_file_input, d, d));

    if (result == nullptr) {
      LOG(ERROR) << "Error while configuring path";
      PyErr_Print();
      return NULL;
    }

    return d;
  }

  // Overridden from ContentHandlerFactory::ManagedDelegate:
  void RunApplication(ShellPtr shell, URLResponsePtr response) override {
    std::unique_ptr<base::ScopedTempDir> temp_dir =
        ExtractApplication(response.Pass());
    base::FilePath directory_path = temp_dir->path();

    PyObject* d = SetupPythonEnvironment(directory_path.value());
    DCHECK(d);

    base::FilePath entry_path = directory_path.Append("__mojo__.py");

    FILE* entry_file = base::OpenFile(entry_path, "r");
    DCHECK(entry_file);

    // Ensure that all created objects are destroyed before calling Py_Finalize.
    {
      // Load the __mojo__.py file into the interpreter. MojoMain hasn't run
      // yet.
      ScopedPyRef result(PyRun_File(entry_file, entry_path.value().c_str(),
                                    Py_file_input, d, d));
      if (result == nullptr) {
        LOG(ERROR) << "Error while loading script";
        PyErr_Print();
        return;
      }

      // Find MojoMain.
      ScopedPyRef py_function(PyMapping_GetItemString(d, kMojoMain));

      if (py_function == NULL) {
        LOG(ERROR) << "Locals size: " << PyMapping_Size(d);
        LOG(ERROR) << "MojoMain not found";
        PyErr_Print();
        return;
      }

      if (PyCallable_Check(py_function)) {
        MojoHandle shell_handle = shell.PassMessagePipe().release().value();
        ScopedPyRef py_input(PyInt_FromLong(shell_handle));
        ScopedPyRef py_arguments(PyTuple_New(1));
        // py_input reference is stolen by py_arguments
        PyTuple_SetItem(py_arguments, 0, py_input.Release());
        // Run MojoMain with shell_handle as the first and only argument.
        ScopedPyRef py_output(PyObject_CallObject(py_function, py_arguments));

        if (!py_output) {
          LOG(ERROR) << "Error while executing MojoMain";
          PyErr_Print();
          return;
        }
      } else {
        LOG(ERROR) << "MojoMain is not callable; it should be a function";
      }
    }
    Py_Finalize();
  }

  std::string CopyToString(ScopedDataPipeConsumerHandle body) {
    std::string body_str;
    bool result = common::BlockingCopyToString(body.Pass(), &body_str);
    DCHECK(result);
    return body_str;
  }

  ContentHandlerFactory content_handler_factory_;

  DISALLOW_COPY_AND_ASSIGN(PythonContentHandler);
};

}  // namespace python
}  // namespace mojo

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(
      new mojo::python::PythonContentHandler());
  return runner.Run(shell_handle);
}
