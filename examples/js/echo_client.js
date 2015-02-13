#!mojo mojo:js_content_handler
// A JavaScript version of the C++ echo example. Connect to the echo_server.js
// application, call it's echoString() method, and then quit. To run this
// application with mojo_shell, set DIR to be the absolute path for this
// directory, then:
//   mojo_shell "file://$DIR/echo_client.js"

define("main", [
  "console",
  "mojo/services/public/js/application",
  "examples/echo/echo.mojom",
], function(console, application, echo) {

  const Application = application.Application;
  const Echo = echo.Echo;

  class EchoClientApp extends Application {
    initialize(args) {
      // The URL of this application is saved by the Application constructor.
      // Assume that the echo_server.js URL is a sibling of echo_client.js.
      // Note also: we could also connect to to the C++ echo service with:
      // var echoServerURL = "mojo:echo_server";
      var echoServerURL =
          this.url.replace("echo_client.js", "echo_server.js");
      var echoServer =
          this.shell.connectToService(echoServerURL, Echo);
      var echoClientApp = this;
      echoServer.echoString("Hello World").then(function(response) {
        console.log("Response: " + response.value);
        echoClientApp.quit();
      });
    }
  }

  return EchoClientApp;
});
