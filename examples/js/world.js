#!mojo:js_content_handler
// See hello.js.
define("main", [
  "console",
  "mojo/services/public/js/application",
], function(console, appModule) {

  class World extends appModule.Application {
    initialize(args) {
      console.log(this.url + ": World");
      this.quit();
    }
  }

  return World;
});
