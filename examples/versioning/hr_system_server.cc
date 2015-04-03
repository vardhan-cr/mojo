// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/versioning/hr_system_server.mojom.h"
#include "mojo/common/weak_binding_set.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_runner.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/public/cpp/bindings/map.h"

namespace mojo {
namespace examples {

class HumanResourceDatabaseImpl : public HumanResourceDatabase {
 public:
  HumanResourceDatabaseImpl() {
    // Pretend that there is already some data in the system.
    EmployeePtr employee(Employee::New());
    employee->employee_id = 1u;
    employee->name = "Homer Simpson";
    employee->department = DEPARTMENT_DEV;
    employee->birthday = Date::New();
    employee->birthday->year = 1955u;
    employee->birthday->month = 5u;
    employee->birthday->day = 12u;

    employees_[1u] = employee.Pass();
  }

  ~HumanResourceDatabaseImpl() override {}

  void AddEmployee(EmployeePtr employee,
                   const AddEmployeeCallback& callback) override {
    uint64_t id = employee->employee_id;
    employees_[id] = employee.Pass();
    callback.Run(true);
  }

  void QueryEmployee(uint64_t id,
                     const QueryEmployeeCallback& callback) override {
    if (employees_.find(id) == employees_.end())
      callback.Run(nullptr);
    callback.Run(employees_[id].Clone());
  }

 private:
  // Obviously this is not the right way to implement a database. The purpose of
  // this example is to demonstrate Mojom definition versioning. Things
  // irrelevant to this purpose are mocked out.
  Map<uint64_t, EmployeePtr> employees_;
};

class HumanResourceSystemServer
    : public ApplicationDelegate,
      public InterfaceFactory<HumanResourceDatabase> {
 public:
  HumanResourceSystemServer() {}

  // From ApplicationDelegate
  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    connection->AddService<HumanResourceDatabase>(this);
    return true;
  }

  // From InterfaceFactory<HumanResourceDatabase>
  void Create(ApplicationConnection* connection,
              InterfaceRequest<HumanResourceDatabase> request) override {
    // All channels will connect to this singleton object, so just
    // add the binding to our collection.
    bindings_.AddBinding(&human_resource_database_impl_, request.Pass());
  }

 private:
  HumanResourceDatabaseImpl human_resource_database_impl_;

  WeakBindingSet<HumanResourceDatabase> bindings_;
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunner runner(
      new mojo::examples::HumanResourceSystemServer());

  return runner.Run(application_request);
}
