#!mojo mojo:js_content_handler

define("main", [
  "mojo/services/public/js/application",
  "services/js/test/echo_service.mojom",
], function(application, echoServiceMojom) {

  const Application = application.Application;
  const EchoService = echoServiceMojom.EchoService;
  var echoApp;

  class EchoServiceImpl {
    echoString(s) {
      return Promise.resolve({value: s});
    }

    // This method is only used by the ShareEchoService test.
    shareEchoService() {
      var echoTargetURL = echoApp.url.replace("echo.js", "echo_target.js");
      var targetSP = echoApp.shell.connectToApplication(echoTargetURL);
      // This Promise resolves after echo_target.js has called the echoString()
      // method defined on the local EchoService implementation. For its part,
      // the target application quits after one successful call to echoString().
      return new Promise(function(resolve) {
        targetSP.provideService(EchoService, function() {
          this.echoString = function(s) {
            resolve({ok: true});
            return Promise.resolve({value: s});
          }
        });
      });
    }

    quit() {
      echoApp.quit();
    }
  }

  class Echo extends Application {
    acceptConnection(url, serviceExchange) {
      echoApp = this;
      serviceExchange.provideService(EchoService, EchoServiceImpl);
    }
  }

  return Echo;
});
