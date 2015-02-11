// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/test_io_thread.h"
#include "mojo/common/common_type_converters.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/interfaces/application/application.mojom.h"
#include "mojo/public/interfaces/application/service_provider.mojom.h"
#include "mojo/public/interfaces/application/shell.mojom.h"
#include "shell/application_manager/application_loader.h"
#include "shell/application_manager/application_manager.h"
#include "shell/domain_socket/net_errors.h"
#include "shell/domain_socket/test_completion_callback.h"
#include "shell/domain_socket/unix_domain_client_socket_posix.h"
#include "shell/external_application_listener.h"
#include "shell/external_application_registrar.mojom.h"
#include "shell/external_application_registrar_connection.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace mojo {
namespace shell {

class NotAnApplicationLoader : public ApplicationLoader {
 public:
  NotAnApplicationLoader() {}
  ~NotAnApplicationLoader() override {}

  void Load(ApplicationManager* application_manager,
            const GURL& url,
            InterfaceRequest<Application> application_request,
            LoadCallback callback) override {
    NOTREACHED();
  }

  void OnApplicationError(ApplicationManager* manager,
                          const GURL& url) override {
    NOTREACHED();
  }
};

class ExternalApplicationListenerTest : public testing::Test {
 public:
  ExternalApplicationListenerTest()
      : io_thread_(base::TestIOThread::kAutoStart) {}
  ~ExternalApplicationListenerTest() override {}

  void SetUp() override {
    listener_.reset(
        new ExternalApplicationListener(main_runner(), io_runner()));
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    socket_path_ = temp_dir_.path().Append(FILE_PATH_LITERAL("socket"));
  }

 protected:
  scoped_refptr<base::SingleThreadTaskRunner> io_runner() {
    return io_thread_.task_runner();
  }
  scoped_refptr<base::SingleThreadTaskRunner> main_runner() {
    return message_loop_.task_runner();
  }
  ExternalApplicationListener* listener() { return listener_.get(); }
  const base::FilePath& socket_path() const { return socket_path_; }

 private:
  base::TestIOThread io_thread_;

  // Note: This must be *after* |io_thread_|, since it needs to be destroyed
  // first (to destroy all bound message pipes).
  base::MessageLoop message_loop_;

  scoped_ptr<ExternalApplicationListener> listener_;

  base::ScopedTempDir temp_dir_;
  base::FilePath socket_path_;

  DISALLOW_COPY_AND_ASSIGN(ExternalApplicationListenerTest);
};

namespace {

class StubShellImpl : public Shell {
 public:
  StubShellImpl(ApplicationPtr application)
      : application_(application.Pass()), binding_(this) {
    ShellPtr shell;
    binding_.Bind(GetProxy(&shell));
    application_->Initialize(shell.Pass(), Array<String>(), "");
  }
  ~StubShellImpl() override {}

 private:
  void ConnectToApplication(const String& requestor_url,
                            InterfaceRequest<ServiceProvider> services,
                            ServiceProviderPtr exposed_services) override {
    application_->AcceptConnection(requestor_url, services.Pass(),
                                   exposed_services.Pass());
  }

  ApplicationPtr application_;
  StrongBinding<Shell> binding_;
};

void DoLocalRegister(const GURL& app_url,
                     const std::vector<std::string>& args,
                     ApplicationPtr application) {
  new StubShellImpl(application.Pass());
}

void ConnectOnIOThread(const base::FilePath& socket_path,
                       scoped_refptr<base::TaskRunner> to_quit,
                       base::Closure quit_callback) {
  ExternalApplicationRegistrarConnection connection(socket_path);
  EXPECT_TRUE(connection.Connect());
  to_quit->PostTask(FROM_HERE, quit_callback);
}

}  // namespace

TEST_F(ExternalApplicationListenerTest, ConnectConnection) {
  listener()->ListenInBackground(socket_path(), base::Bind(&DoLocalRegister));
  listener()->WaitForListening();
  base::RunLoop run_loop;
  io_runner()->PostTask(
      FROM_HERE, base::Bind(&ConnectOnIOThread, socket_path(), main_runner(),
                            run_loop.QuitClosure()));
  run_loop.Run();
}

namespace {

class QuitLoopOnConnectApp : public ApplicationDelegate {
 public:
  QuitLoopOnConnectApp(const std::string& url,
                       scoped_refptr<base::TaskRunner> loop,
                       base::Closure quit_callback)
      : url_(url), to_quit_(loop), quit_callback_(quit_callback) {}

 private:
  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    DVLOG(1) << url_ << " accepting connection from "
             << connection->GetRemoteApplicationURL();
    to_quit_->PostTask(FROM_HERE, quit_callback_);
    return true;
  }

  const std::string url_;
  scoped_refptr<base::TaskRunner> to_quit_;
  base::Closure quit_callback_;
};

class FakeExternalApplication {
 public:
  explicit FakeExternalApplication(const std::string& url) : url_(url) {}

  void ConnectSynchronously(const base::FilePath& socket_path) {
    connection_.reset(new ExternalApplicationRegistrarConnection(socket_path));
    EXPECT_TRUE(connection_->Connect());
  }

  // application_delegate is the the actual implementation to be registered.
  void Register(scoped_ptr<ApplicationDelegate> application_delegate,
                const base::Closure& register_complete_callback) {
    connection_->Register(
        GURL(url_), std::vector<std::string>(),
        base::Bind(&FakeExternalApplication::OnRegister, base::Unretained(this),
                   register_complete_callback));
    application_delegate_ = application_delegate.Pass();
  }

  void ConnectToAppByUrl(std::string app_url) {
    ServiceProviderPtr sp;
    application_impl_->WaitForInitialize();
    application_impl_->shell()->ConnectToApplication(app_url, GetProxy(&sp),
                                                     nullptr);
  }

  const std::string& url() { return url_; }

 private:
  void OnRegister(const base::Closure& callback,
                  InterfaceRequest<Application> application_request) {
    application_impl_.reset(new ApplicationImpl(application_delegate_.get(),
                                                application_request.Pass()));
    callback.Run();
  }

  const std::string url_;
  scoped_ptr<ApplicationDelegate> application_delegate_;
  scoped_ptr<ApplicationImpl> application_impl_;

  scoped_ptr<ExternalApplicationRegistrarConnection> connection_;
};

void ConnectToApp(FakeExternalApplication* connector,
                  FakeExternalApplication* connectee) {
  connector->ConnectToAppByUrl(connectee->url());
}

void ConnectAndRegisterOnIOThread(const base::FilePath& socket_path,
                                  scoped_refptr<base::TaskRunner> loop,
                                  base::Closure quit_callback,
                                  FakeExternalApplication* connector,
                                  FakeExternalApplication* connectee) {
  // Connect the first app to the registrar.
  connector->ConnectSynchronously(socket_path);
  // connector will use this implementation of the Mojo Application interface
  // once registration complete.
  scoped_ptr<QuitLoopOnConnectApp> connector_app(
      new QuitLoopOnConnectApp(connector->url(), loop, quit_callback));

  // Since connectee won't be ready when connector is done registering, pass
  // in a do-nothing callback.
  connector->Register(connector_app.Pass(), base::Bind(&base::DoNothing));

  // Connect the second app to the registrar.
  connectee->ConnectSynchronously(socket_path);

  scoped_ptr<QuitLoopOnConnectApp> connectee_app(
      new QuitLoopOnConnectApp(connectee->url(), loop, quit_callback));
  // After connectee is successfully registered, connector should be
  // able to connect to is by URL. Pass in a callback to attempt the
  // app -> app connection.
  connectee->Register(connectee_app.Pass(),
                      base::Bind(&ConnectToApp, connector, connectee));
}

void DestroyOnIOThread(scoped_ptr<FakeExternalApplication> doomed1,
                       scoped_ptr<FakeExternalApplication> doomed2) {
}

}  // namespace

// Create two external applications, have them discover and connect to
// the registrar, and then have one app connect to the other by URL.
TEST_F(ExternalApplicationListenerTest, ConnectTwoExternalApplications) {
  ApplicationManager::Delegate delegate;
  ApplicationManager application_manager(&delegate);
  application_manager.set_default_loader(
      scoped_ptr<ApplicationLoader>(new NotAnApplicationLoader));

  listener()->ListenInBackground(
      socket_path(),
      base::Bind(&ApplicationManager::RegisterExternalApplication,
                 base::Unretained(&application_manager)));
  listener()->WaitForListening();

  // Create two external apps.
  scoped_ptr<FakeExternalApplication> supersweet_app(
      new FakeExternalApplication("http://my.supersweet.app"));
  scoped_ptr<FakeExternalApplication> awesome_app(
      new FakeExternalApplication("http://my.awesome.app"));

  // Connecting and talking to the registrar has to happen on the IO thread.
  base::RunLoop run_loop;
  io_runner()->PostTask(FROM_HERE,
                        base::Bind(&ConnectAndRegisterOnIOThread, socket_path(),
                                   main_runner(), run_loop.QuitClosure(),
                                   supersweet_app.get(), awesome_app.get()));
  run_loop.Run();

  // The apps need to be destroyed on the thread where they did socket stuff.
  io_runner()->PostTask(
      FROM_HERE, base::Bind(&DestroyOnIOThread, base::Passed(&supersweet_app),
                            base::Passed(&awesome_app)));
}

}  // namespace shell
}  // namespace mojo
