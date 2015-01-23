#!mojo mojo:js_content_handler

define("main", [
  "mojo/public/js/bindings",
  "mojo/services/public/js/application",
  "services/js/test/pingpong_service.mojom",
], function(bindings, application, pingPongServiceMojom) {

  const ProxyBindings = bindings.ProxyBindings;
  const StubBindings = bindings.StubBindings;
  const Application = application.Application;
  const PingPongService = pingPongServiceMojom.PingPongService;

  class PingPongServiceImpl {
    constructor(app) {
      this.app = app;
    }
    setClient(client) {
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
        var pingTargetService =
            app.shell.connectToService(url, PingPongService);
        pingTargetService.setClient(pingTargetClient);
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
        pingTargetService.setClient(pingTargetClient);
        for(var i = 0; i <= count; i++)
          pingTargetService.ping(i);
      });
    }

    // This method is only used by the GetTargetService test.
    getPingPongService(pingPongServiceProxy) {
      ProxyBindings(pingPongServiceProxy).setLocalDelegate(
          new PingPongServiceImpl(this));
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
