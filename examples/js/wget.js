#!mojo:js_content_handler
// Demonstrate use of the Mojo network service to load a URL. To run this
// application with mojo_shell, set DIR to be the absolute path for this
// directory, then:
//   mojo_shell "file://$DIR/wget.js http://www.google.com"
// Substitute any URL for google.com. To keep the noise down, this app
// only displays the number of bytes read and a little http header info.

define("main", [
  "console",
  "mojo/public/js/core",
  "mojo/services/public/interfaces/network/network_service.mojom",
  "mojo/services/public/interfaces/network/url_loader.mojom",
  "mojo/services/public/js/shell",
  "services/js/app_bridge",
], function(console,
            coreModule,
            netModule,
            loaderModule,
            shellModule,
            appModule) {

  function Application(appShell, url) {
    this.shell = new shellModule.Shell(appShell);
  }

  Application.prototype.initialize = function(args) {
    if (args.length != 2) {
      console.log("Expected URL argument");
      return;
    }

    var netService = this.shell.connectToService(
        "mojo:network_service", netModule.NetworkService);

    var urlLoader = new loaderModule.URLLoader.proxyClass();
    netService.createURLLoader(urlLoader);

    var urlRequest = new loaderModule.URLRequest({
      url: args[1],
      method: "GET",
      auto_follow_redirects: true
    });

    urlLoader.start(urlRequest).then(function(result) {
      console.log("url => " + result.response["url"]);
      console.log("status_line => " + result.response["status_line"]);
      console.log("mime_type => " + result.response["mime_type"]);

      coreModule.drainData(result.response.body).then(
        function(result) {
          console.log("read " + result.buffer.byteLength + " bytes");
        },
        function (reject) {
          console.log("failed " + reject);
        })
        .then(appModule.quit);
    });
  }

  Application.prototype.acceptConnection = function(url, serviceProvider) {
  }

  return Application;
});
