#!mojo mojo:js_content_handler

define("main", [
  "mojo/services/public/js/application",
  "services/js/test/echo_service.mojom"
], function(application, echoServiceMojom) {

  const Application = application.Application;
  const EchoService = echoServiceMojom.EchoService;

  class Echo extends Application {
    acceptConnection(url, serviceProvider) {
      var app = this;
      function EchoServiceImpl(client) {
        this.echoString = function(s) {
          if (s == "quit")
            app.quit();
          return Promise.resolve({value: s});
        };
      }
      serviceProvider.provideService(EchoService, EchoServiceImpl);
    }
  }

  return Echo;
});
