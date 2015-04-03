// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/versioning/hr_system_client.mojom.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_runner.h"
#include "mojo/public/cpp/environment/logging.h"
#include "mojo/public/cpp/utility/run_loop.h"

namespace mojo {
namespace examples {

void LogEmployee(const EmployeePtr& employee) {
  if (!employee) {
    MOJO_LOG(INFO) << "Employee[null]";
    return;
  }

  MOJO_LOG(INFO) << "Employee[employee_id=" << employee->employee_id
                 << ", name=" << employee->name
                 << ", department=" << employee->department << "]";
}

class HumanResourceSystemClient : public ApplicationDelegate {
 public:
  ~HumanResourceSystemClient() override {}

  void Initialize(ApplicationImpl* app) override {
    app->ConnectToService("mojo:versioning_hr_system_server", &database_);

    MOJO_LOG(INFO) << "Query an existing employee with ID 1...";
    database_->QueryEmployee(
        1u, [](EmployeePtr employee) { LogEmployee(employee); });
    database_.WaitForIncomingMethodCall();

    EmployeePtr new_employee(Employee::New());
    new_employee->employee_id = 2u;
    new_employee->name = "Marge Simpson";
    new_employee->department = DEPARTMENT_SALES;

    MOJO_LOG(INFO) << "Add a new employee with the following information:";
    LogEmployee(new_employee);
    database_->AddEmployee(new_employee.Pass(), [](bool success) {
      MOJO_LOG(INFO) << "success: " << success;
    });
    database_.WaitForIncomingMethodCall();

    MOJO_LOG(INFO) << "Query the newly added employee with ID 2...";
    database_->QueryEmployee(2u, [](EmployeePtr employee) {
      LogEmployee(employee);
      RunLoop::current()->Quit();
    });
  }

 private:
  HumanResourceDatabasePtr database_;
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunner runner(new mojo::examples::HumanResourceSystemClient);
  return runner.Run(application_request);
}
