#!mojo:js_content_handler
// Demonstrate one JS Mojo application connecting to another to emit "hello
// world". To run this application with mojo_shell, set DIR to be the absolute
// path for this directory, then:
//   mojo_shell "file://$DIR/hello.js file://$DIR/world.js"
// Launches the Mojo hello.js application which connects to the application
// URL specified as a Mojo application argument, world.js in this case.

define("main", [
  "console",
  "mojo/services/public/js/application",
], function(console, appModule) {

  class Hello extends appModule.Application {
    initialize(args) {
      if (args.length != 2) {
        console.log("Expected hello.js URL argument");
        return;
      }
      console.log(this.url + ": Hello");
      this.shell.connectToApplication(args[1]);
      this.quit();
    }
  }

  return Hello;
});
