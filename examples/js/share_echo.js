#!mojo mojo:js_content_handler
// Demonstrate using the ServiceProvider returned by the Shell
// connectToApplication() method to provide a service to the
// target application and to request a service from the target
// application. To run this application with mojo_shell:
//   mojo_shell file://full/path/to/share_echo.js

define("main", [
  "console",
  "mojo/services/public/js/application",
  "examples/echo/echo.mojom",
], function(console, application, echo) {

  const Application = application.Application;
  const Echo = echo.Echo;

  var shareEchoApp;
  var everythingDone = {providedService: false, requestedService: false};

  // This application will quit after the EchoImpl has been used once
  // and the target Echo application has responded once.
  function quitIfEverythingIsDone(doneNow) {
    everythingDone[doneNow] = true;
    if (everythingDone.providedService && everythingDone.requestedService)
      shareEchoApp.quit();
  }

  class EchoImpl {
    echoString(s) {
      quitIfEverythingIsDone("providedService");
      return Promise.resolve({value: "ShareEcho: " + s});
    }
  }

  class ShareEcho extends Application {
    initialize(args) {
      // The Application object is a singleton. Remember it.
      shareEchoApp = this;

      var shareEchoTargetURL =
          this.url.replace("share_echo.js", "share_echo_target.js");

      // The value of targetSP is-a JS ServiceProvider that's connected to the
      // share_echo_target.js application. We provide our implementation of
      // Echo to the share_echo_target.js application and request its
      // Echo implementation.
      var targetSP = this.shell.connectToApplication(shareEchoTargetURL);

      // Make our implementation of Echo available to the other end.
      // Note that we pass the class, not an instance.
      targetSP.provideService(Echo, EchoImpl);

      // Get the implementation at the other end, call it, and prepare for
      // the async callback.
      var echoService = targetSP.requestService(Echo);
      echoService.echoString("ShareEcho").then(function(response) {
        console.log(response.value);
        quitIfEverythingIsDone("requestedService");
      });
    }
  }

  return ShareEcho;
});
