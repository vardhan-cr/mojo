#!mojo:js_content_handler

define("main", [
  "mojo/services/public/js/service_provider",
  "services/js/app_bridge",
  "services/js/test/pingpong_service.mojom",
  "mojo/services/public/js/shell","console",
], function(spModule, appModule, ppModule, shellModule,console) {

  function PingPongServiceImpl(client) {
    this.ping = function(value) {
      client.pong(value + 1);
      if (value == 999)
        appModule.quit();
    }
  }

  function Application(appShell, url) {
    this.shell = new shellModule.Shell(appShell);
    this.serviceProviders = [];
  }

  Application.prototype.initialize = function(args) {
  }

  Application.prototype.acceptConnection = function(url, spHandle) {
    var serviceProvider =  new spModule.ServiceProvider(spHandle);
    serviceProvider.provideService(
        ppModule.PingPongService, PingPongServiceImpl);
    this.serviceProviders.push(serviceProvider);
  }

  return Application;
});
