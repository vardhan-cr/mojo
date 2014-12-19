#!mojo:js_content_handler

define("main", [
  "mojo/services/public/js/application",
  "services/js/test/pingpong_service.mojom",
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
    }
    quit() {
      this.app.quit();
    }
  }

  class PingPongTarget extends Application {
    acceptConnection(url, serviceProvider) {
      serviceProvider.provideService(
          PingPongService, PingPongServiceImpl.bind(null, this));
    }
  }

  return PingPongTarget;
});
