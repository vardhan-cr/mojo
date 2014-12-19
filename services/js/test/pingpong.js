#!mojo:js_content_handler

define("main", [
  "mojo/services/public/js/application",
  "services/js/test/pingpong_service.mojom"
], function(application, pingPongServiceMojom) {

  const Application = application.Application;
  const PingPongService = pingPongServiceMojom.PingPongService;

  class PingPongServiceImpl {
    constructor(app, client) {
      this.app = app;
      this.client = client;
    }
    ping(value) {
      this.client.pong(value + 1);
    };
    quit() {
      this.app.quit();
    }

    // This method is only used by the PingTargetURL test.
    pingTargetURL(url, count) {
      var app = this.app;
      return new Promise(function(resolve) {
        var pingTargetClient = {
          pong: function(value) {
            if (value == count) {
              pingTargetService.quit();
              resolve({ok: true});
            }
          }
        };
        var pingTargetService = app.shell.connectToService(
            url, PingPongService, pingTargetClient);
        for(var i = 0; i <= count; i++)
          pingTargetService.ping(i);
      });
    }

    // This method is only used by the PingTargetService test.
    pingTargetService(pingTargetService, count) {
      return new Promise(function(resolve) {
        var pingTargetClient = {
          pong: function(value) {
            if (value == count) {
              pingTargetService.quit();
              resolve({ok: true});
            }
          }
        };
        pingTargetService.local$ = pingTargetClient;
        for(var i = 0; i <= count; i++)
          pingTargetService.ping(i);
      });
    }

    // This method is only used by the GetTargetService test.
    getPingPongService(clientProxy) {
      clientProxy.local$ = new PingPongServiceImpl(this, clientProxy);
    }
  }

  class PingPong extends Application {
    acceptConnection(url, serviceProvider) {
      serviceProvider.provideService(
          PingPongService, PingPongServiceImpl.bind(null, this));
    }
  }

  return PingPong;
});
