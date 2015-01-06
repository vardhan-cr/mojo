#!mojo:js_content_handler
// Demonstrate sharing services via application ServiceProviders.
// This application is launched by share_echo.js. It both provides
// and requests an EchoService implementation.
//
define("main", [
  "console",
  "mojo/services/public/js/application",
  "examples/echo/echo_service.mojom",
], function(console, application, echo) {

  var Application = application.Application;
  var EchoService = echo.EchoService;

  class EchoServiceImpl {
    echoString(s) {
      return Promise.resolve({value: "ShareEchoTarget: " + s});
    }
  }

  class ShareEchoTarget extends Application {
    acceptConnection(initiatorURL, shareEchoSP) {
      // The shareEchoSP parameter is-a JS ServiceProvider that's
      // connected to the share_echo.js application. We can request the
      // share_echo.js implementation of EchoService and provide the
      // share_echo.js application with our own EchoService implementation.
      var echoService = shareEchoSP.requestService(EchoService);
      echoService.echoString("ShareEchoTarget").then(function(response) {
        console.log(response.value);
      });
      shareEchoSP.provideService(EchoService, EchoServiceImpl);
    }
  }

  return ShareEchoTarget;
});
