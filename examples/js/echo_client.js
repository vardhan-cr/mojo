#!mojo mojo:js_content_handler
// A JavaScript version of the C++ echo example. Connect to the echo_service.js
// application, call it's echoString() method, and then quit. To run this
// application with mojo_shell, set DIR to be the absolute path for this
// directory, then:
//   mojo_shell "file://$DIR/echo_client.js"

define("main", [
  "console",
  "mojo/services/public/js/application",
  "examples/echo/echo_service.mojom",
], function(console, application, echo) {

  const Application = application.Application;
  const EchoService = echo.EchoService;

  class EchoClientApp extends Application {
    initialize(args) {
      // The URL of this application is saved by the Application constructor.
      // Assume that the echo_service.js URL is a sibling of echo_client.js.
      // Note also: we could also connect to to the C++ echo service with:
      // var echoServiceURL = "mojo:echo_service";
      var echoServiceURL =
          this.url.replace("echo_client.js", "echo_service.js");
      var echoService =
          this.shell.connectToService(echoServiceURL, EchoService);
      var echoServiceApp = this;
      echoService.echoString("Hello World").then(function(response) {
        console.log("Response: " + response.value);
        echoServiceApp.quit();
      });
    }
  }

  return EchoClientApp;
});
