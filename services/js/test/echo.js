#!mojo:js_content_handler

define("main", [
  "mojo/services/public/js/application",
  "services/js/test/echo_service.mojom",
], function(appModule, echoModule) {

  class Echo extends appModule.Application {
    acceptConnection(url, serviceProvider) {
      var app = this;
      function EchoServiceImpl(client) {
        this.echoString = function(s) {
          if (s == "quit")
            app.quit();
          return Promise.resolve({value: s});
        };
      }
      serviceProvider.provideService(echoModule.EchoService, EchoServiceImpl);
    }
  }

  return Echo;
});
