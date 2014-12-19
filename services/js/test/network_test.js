#!mojo:js_content_handler

define("main", [
  "mojo/services/public/js/application",
  "mojo/public/js/core",
  "mojo/services/network/public/interfaces/network_service.mojom",
  "mojo/services/network/public/interfaces/url_loader.mojom",
  "services/js/test/network_test_service.mojom",
], function(application,
            core,
            networkServiceMojom,
            urlLoaderMojom,
            networkTestServiceMojom) {

  const Application = application.Application;
  const NetworkService = networkServiceMojom.NetworkService;
  const URLRequest = urlLoaderMojom.URLRequest;
  const NetworkTestService = networkTestServiceMojom.NetworkTestService;

  class NetworkTestServiceImpl {
    constructor(app) {
      this.app = app;
      var netService = app.shell.connectToService(
          "mojo:network_service", NetworkService);
      var urlLoaderClient = {};
      netService.createURLLoader(urlLoaderClient);
      this.urlLoader = urlLoaderClient.remote$;
    }

    getFileSize(url) {
      var impl = this;
      return new Promise(function(resolve) {
        var urlRequest = new URLRequest({
          url: url,
          method: "GET",
          auto_follow_redirects: true
        });
        impl.urlLoader.start(urlRequest).then(function(result) {
          core.drainData(result.response.body).then(
            function(result) {
              resolve({ok: true, size: result.buffer.byteLength});
            });
        }).catch(function() {
          resolve({ok: false});
        });
      });
    }

    quit() {
      this.app.quit();
    }
  }

  class NetworkTest extends Application {
    acceptConnection(url, serviceProvider) {
      serviceProvider.provideService(
          NetworkTestService,
          NetworkTestServiceImpl.bind(null, this));
    }
  }

  return NetworkTest;
});
