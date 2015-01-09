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
  var echoServiceApp;

  class EchoServiceImpl {
    echoString(s) {
      echoServiceApp.quit();
      return Promise.resolve({value: s});
    }
  }

  class EchoServiceApp extends Application {
    initialize(args) {
      echoServiceApp = this;
    }

    acceptConnection(initiatorURL, initiatorSP) {
      initiatorSP.provideService(EchoService, EchoServiceImpl);
    }
  }

  return EchoServiceApp;
});
