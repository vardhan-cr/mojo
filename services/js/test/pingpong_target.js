#!mojo:js_content_handler

define("main", [
  "mojo/services/public/js/application",
  "services/js/test/pingpong_service.mojom",
], function(appModule, ppModule) {

  class PingPongServiceImpl {
    constructor(app, client) {
      this.app = app;
      this.client = client;
    }

    ping(value) {
      this.client.pong(value + 1);
    }

    quit() {
      this.app.quit();
    }
  }

  class PingPongTarget extends appModule.Application {
    acceptConnection(url, serviceProvider) {
      serviceProvider.provideService(
          ppModule.PingPongService, PingPongServiceImpl.bind(null, this));
    }
  }

  return PingPongTarget;
});
