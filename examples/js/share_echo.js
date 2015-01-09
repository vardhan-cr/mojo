#!mojo mojo:js_content_handler
// Demonstrate using the ServiceProvider returned by the Shell
// connectToApplication() method to provide a service to the
// target application and to request a service from the target
// application. To run this application with mojo_shell, set DIR
// to be the absolute path for this directory, then:
//   mojo_shell "file://$DIR/share_echo.js file://$DIR/share_echo_target.js"

define("main", [
  "console",
  "mojo/services/public/js/application",
  "examples/echo/echo_service.mojom",
], function(console, application, echo) {

  const Application = application.Application;
  const EchoService = echo.EchoService;

  var shareEchoApp;
  var everythingDone = {providedService: false, requestedService: false};

  // This application will quit after the EchoServiceImpl has been used once
  // and the EchoService we requested has responded once.
  function quitIfEverythingIsDone(doneNow) {
    everythingDone[doneNow] = true;
    if (everythingDone.providedService && everythingDone.requestedService)
      shareEchoApp.quit();
  }

  class EchoServiceImpl {
    echoString(s) {
      quitIfEverythingIsDone("providedService");
      return Promise.resolve({value: "ShareEcho: " + s});
    }
  }

  class ShareEcho extends Application {
    initialize(args) {
      if (args.length != 2) {
        console.log("Expected URL argument");
        return;
      }
      shareEchoApp = this;
      var shareEchoTargetURL = args[1];
      // The value of targetSP is-a JS ServiceProvider that's connected to the
      // share_echo_target.js application. We provide our implementation of
      // EchoService to the share_echo_target.js application and request its
      // EchoService implementation.
      var targetSP = this.shell.connectToApplication(shareEchoTargetURL);
      targetSP.provideService(EchoService, EchoServiceImpl);
      var shareEchoTargetApp = this;
      var echoService = targetSP.requestService(EchoService);
      echoService.echoString("ShareEcho").then(function(response) {
        console.log(response.value);
        quitIfEverythingIsDone("requestedService");
      });
    }
  }

  return ShareEcho;
});
