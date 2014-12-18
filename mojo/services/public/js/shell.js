// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

define("mojo/services/public/js/shell", [
  "mojo/public/js/core",
  "mojo/public/js/connection",
  "mojo/public/interfaces/application/shell.mojom",
  "mojo/public/interfaces/application/service_provider.mojom",
  "mojo/services/public/js/service_provider"
], function(coreModule,
            connectionModule,
            shellInterfaceModule,
            spInterfaceModule,
            spModule) {

  class Shell {
    constructor(shellHandle, app) {
      this.shellHandle = shellHandle;
      this.proxy = connectionModule.bindProxyHandle(
          shellHandle,
          shellInterfaceModule.Shell.client,
          shellInterfaceModule.Shell);
      this.proxy.local$ = app; // The app is the shell's client.
      // TODO: call this serviceProviders_
      this.applications_ = new Map();
    }

    connectToApplication(url) {
      var application = this.applications_.get(url);
      if (application)
        return application;

      var returnValue = {};
      this.proxy.connectToApplication(url, returnValue);
      application = new spModule.ServiceProvider(returnValue.remote$);
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
      coreModule.close(this.shellHandle);
    }
  }

  var exports = {};
  exports.Shell = Shell;
  return exports;
});
