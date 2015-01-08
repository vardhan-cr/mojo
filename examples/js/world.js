#!mojo mojo:js_content_handler
// See hello.js.
define("main", [
  "console",
  "mojo/services/public/js/application",
], function(console, application) {

  var Application = application.Application;

  class World extends Application {
    initialize(args) {
      console.log(this.url + ": World");
      this.quit();
    }
  }

  return World;
});
