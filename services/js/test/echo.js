#!mojo:js_content_handler

define("main", [
  "console",
  "mojo/services/public/js/service-provider",
  "services/js/test/echo_service.mojom",
  "services/js/app-bridge",
], function(console, spModule, echoModule, appModule) {

  function Application(shell, url) {
    this.shell = shell;
    this.serviceProviders = [];
  }

  Application.prototype.initialize = function(args) {
  }

  Application.prototype.acceptConnection = function(url, spHandle) {
    function EchoServiceImpl(client) {
      this.echoString = function(s) {
        if (s == "quit")
          appModule.quit();
        return Promise.resolve({value: s});
      };
    }
    var serviceProvider =  new spModule.ServiceProvider(spHandle);
    serviceProvider.provideService(echoModule.EchoService, EchoServiceImpl);
    this.serviceProviders.push(serviceProvider);
  }

  return Application;
});
