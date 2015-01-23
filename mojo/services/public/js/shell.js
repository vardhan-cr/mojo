// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

define("mojo/services/public/js/shell", [
  "mojo/public/js/bindings",
  "mojo/public/js/core",
  "mojo/public/js/connection",
  "mojo/public/interfaces/application/shell.mojom",
  "mojo/public/interfaces/application/service_provider.mojom",
  "mojo/services/public/js/service_provider","console",
], function(bindings, core, connection, shellMojom, spMojom, sp, console) {

  const ProxyBindings = bindings.ProxyBindings;
  const StubBindings = bindings.StubBindings;
  const ServiceProvider = sp.ServiceProvider;
  const ServiceProviderInterface = spMojom.ServiceProvider;
  const ShellInterface = shellMojom.Shell;

  class Shell {
    constructor(shellHandle, app) {
      this.shellHandle = shellHandle;
      this.shellProxy = connection.bindProxyHandle(
          shellHandle, ShellInterface.client, ShellInterface);

      ProxyBindings(this.shellProxy).setLocalDelegate(app);
      this.applications_ = new Map();
    }

    connectToApplication(url) {
      var application = this.applications_.get(url);
      if (application)
        return application;

      var application = new ServiceProvider();
      this.shellProxy.connectToApplication(url,
          function(services) {
            application.proxy = services;
          },
          application);
      this.applications_.set(url, application);
      return application;
    }

    connectToService(url, service, client) {
      return this.connectToApplication(url).requestService(service, client);
    };

    close() {
      this.applications_.forEach(function(application, url) {
        application.close();
      });
      this.applications_.clear();
      core.close(this.shellHandle);
    }
  }

  var exports = {};
  exports.Shell = Shell;
  return exports;
});
