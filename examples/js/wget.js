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
  "mojo/public/interfaces/application/service_provider.mojom",
], function(console, core, net, loader, sp) {

  function Application(shell, url) {
    this.shell = shell;
  }

  Application.prototype.initialize = function(args) {
    if (args.length != 2) {
      console.log("Expected URL argument");
      return;
    }

    var netServiceApp = new sp.ServiceProvider.proxyClass;
    this.shell.connectToApplication("mojo:network_service", netServiceApp);

    var netService = new net.NetworkService.proxyClass();
    // TODO(hansmuller): eliminate this step.
    var handle = netService.getConnection$().messagePipeHandle;
    netServiceApp.connectToService(net.NetworkService.name, handle);

    var urlLoader = new loader.URLLoader.proxyClass();
    netService.createURLLoader(urlLoader);

    var urlRequest = new loader.URLRequest({
      url: args[1],
      method: "GET",
      auto_follow_redirects: true
    });

    urlLoader.start(urlRequest).then(function(result) {
      console.log("url => " + result.response["url"]);
      console.log("status_line => " + result.response["status_line"]);
      console.log("mime_type => " + result.response["mime_type"]);

      core.drainData(result.response.body).then(
        function(result) {
          console.log("read " + result.buffer.byteLength + " bytes");
        },
        function (reject) {
          console.log("failed " + reject);
        });
    });
  }

  Application.prototype.acceptConnection = function(url, serviceProvider) {
  }

  return Application;
});
