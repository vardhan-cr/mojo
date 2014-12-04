#!mojo:js_content_handler

define("main", [
  "mojo/services/public/js/service_provider",
  "services/js/app_bridge",
  "services/js/test/pingpong_service.mojom",
  "mojo/services/public/js/shell",
], function(spModule, appModule, ppModule, shellModule) {

  var shell;

  function PingPongServiceImpl(client) {
    this.ping = function(value) {
      if (value == 999)
        appModule.quit();
      client.pong(value + 1);
    };
    // This method is only used by the PingPongTargetApp test.
    this.pingTarget = function(pingTargetURL, count) {
      return new Promise(function(resolve) {
        var pingTargetClient = {
          pong: function(value) {
            if (value == count) {
              pingTargetService.ping(999); // Quit pingTargetService.
              resolve({ok: true});
              appModule.quit();
            }
          }
        };
        var pingTargetService = shell.connectToService(
            pingTargetURL, ppModule.PingPongService, pingTargetClient);
        for(var i = 0; i <= count; i++)
          pingTargetService.ping(i);
      });
    }
  }

  function Application(appShell, url) {
    shell = new shellModule.Shell(appShell);
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
