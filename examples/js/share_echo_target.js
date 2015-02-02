#!mojo mojo:js_content_handler
// Demonstrate sharing services via application ServiceProviders.
// This application is launched by share_echo.js. It both provides and
// requests an Echo implementation. See share_echo.js for instructions.

define("main", [
  "console",
  "mojo/services/public/js/application",
  "examples/echo/echo.mojom",
], function(console, application, echo) {

  const Application = application.Application;
  const Echo = echo.Echo;

  var shareEchoTargetApp;
  var everythingDone = {providedService: false, requestedService: false};

  // This application will quit after the EchoImpl has been used once
  // and the target Echo application has responded once.
  function quitIfEverythingIsDone(doneNow) {
    everythingDone[doneNow] = true;
    if (everythingDone.providedService && everythingDone.requestedService)
      shareEchoTargetApp.quit();
  }

  class EchoImpl {
    echoString(s) {
      quitIfEverythingIsDone("providedService");
      return Promise.resolve({value: "ShareEchoTarget: " + s});
    }
  }

  class ShareEchoTarget extends Application {
    initialize(args) {
      shareEchoTargetApp = this;
    }

    acceptConnection(initiatorURL, shareEchoServiceExchange) {
      // The shareEchoServiceExchange parameter is-a JS ServiceExchange
      // that's connected to the share_echo.js application. We can
      // request the share_echo.js implementation of Echo and provide
      // the share_echo.js application with our own Echo implementation.
      var echoService = shareEchoServiceExchange.requestService(Echo);
      echoService.echoString("ShareEchoTarget").then(function(response) {
        console.log(response.value);
        quitIfEverythingIsDone("requestedService");
      });
      shareEchoServiceExchange.provideService(Echo, EchoImpl);
    }
  }

  return ShareEchoTarget;
});
