#!mojo:js_content_handler

define("main", [
  "mojo/services/public/js/application",
  "mojo/public/js/core",
  "mojo/services/network/public/interfaces/network_service.mojom",
  "mojo/services/network/public/interfaces/url_loader.mojom",
  "services/js/test/network_test_service.mojom",
], function(appModule, coreModule, netModule, loaderModule, ntsModule) {

  class NetworkTestServiceImpl {
    constructor(app) {
      this.app = app;
      var netService = app.shell.connectToService(
        "mojo:network_service", netModule.NetworkService);
      var urlLoaderClient = {};
      netService.createURLLoader(urlLoaderClient);
      this.urlLoader = urlLoaderClient.remote$;
    }

    getFileSize(url) {
      var impl = this;
      return new Promise(function(resolve) {
        var urlRequest = new loaderModule.URLRequest({
          url: url,
          method: "GET",
          auto_follow_redirects: true
        });
        impl.urlLoader.start(urlRequest).then(function(result) {
          coreModule.drainData(result.response.body).then(
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

  class NetworkTest extends appModule.Application {
    acceptConnection(url, serviceProvider) {
      serviceProvider.provideService(
          ntsModule.NetworkTestService,
          NetworkTestServiceImpl.bind(null, this));
    }
  }

  return NetworkTest;
});
