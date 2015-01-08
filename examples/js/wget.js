#!mojo:js_content_handler
// Demonstrate use of the Mojo network service to load a URL. To run this
// application with mojo_shell, set DIR to be the absolute path for this
// directory, then:
//   mojo_shell "file://$DIR/wget.js http://www.google.com"
// Substitute any URL for google.com. To keep the noise down, this app
// only displays the number of bytes read and a little http header info.

define("main", [
  "console",
  "mojo/services/public/js/application",
  "mojo/public/js/core",
  "mojo/services/network/public/interfaces/network_service.mojom",
  "mojo/services/network/public/interfaces/url_loader.mojom",
], function(console, application, core, network, loader) {

  var Application = application.Application;
  var NetworkService = network.NetworkService;
  var URLRequest = loader.URLRequest;

  class WGet extends Application {
    initialize(args) {
      if (args.length != 2) {
        console.log("Expected URL argument");
        return;
      }

      var netService = this.shell.connectToService(
        "mojo:network_service", NetworkService);

      var urlLoader;
      netService.createURLLoader(function(urlLoaderProxy) {
        urlLoader = urlLoaderProxy;
      });

      var urlRequest = new URLRequest({
        url: args[1],
        method: "GET",
        auto_follow_redirects: true
      });

      var app = this;
      urlLoader.start(urlRequest).then(function(result) {
        console.log("url => " + result.response["url"]);
        console.log("status_line => " + result.response["status_line"]);
        console.log("mime_type => " + result.response["mime_type"]);

        core.drainData(result.response.body).then(
          function(result) {
            console.log("read " + result.buffer.byteLength + " bytes");
          })
          .then(function() {
            app.quit();
          });
      });
    }
  }

  return WGet;
});
