// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/run_loop.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/services/view_manager/public/cpp/view.h"
#include "mojo/services/view_manager/public/cpp/view_manager_client_factory.h"
#include "mojo/services/view_manager/public/cpp/view_manager_delegate.h"
#include "mojo/services/window_manager/public/interfaces/window_manager.mojom.h"

namespace mojo {
namespace {

// TestApplication's view is embedded by the window manager.
class TestApplication : public ApplicationDelegate, public ViewManagerDelegate {
 public:
  TestApplication() : root_(nullptr) {}
  ~TestApplication() override {}

  View* root() const { return root_; }

  void set_embed_callback(const base::Closure& callback) {
    embed_callback_ = callback;
  }

 private:
  // ApplicationDelegate:
  virtual void Initialize(mojo::ApplicationImpl* app) override {
    view_manager_client_factory_.reset(
        new mojo::ViewManagerClientFactory(app->shell(), this));
  }

  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    connection->AddService(view_manager_client_factory_.get());
    return true;
  }

  // ViewManagerDelegate:
  void OnEmbed(View* root,
               ServiceProviderImpl* exported_services,
               scoped_ptr<ServiceProvider> imported_services) override {
    root_ = root;
    embed_callback_.Run();
  }
  void OnViewManagerDisconnected(ViewManager* view_manager) override {}

  View* root_;
  base::Closure embed_callback_;
  scoped_ptr<mojo::ViewManagerClientFactory> view_manager_client_factory_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(TestApplication);
};

class WindowManagerApplicationTest : public test::ApplicationTestBase {
 public:
  WindowManagerApplicationTest() {}
  ~WindowManagerApplicationTest() override {}

 protected:
  // ApplicationTestBase:
  void SetUp() override {
    ApplicationTestBase::SetUp();
    application_impl()->ConnectToService("mojo:window_manager",
                                         &window_manager_);
  }
  ApplicationDelegate* GetApplicationDelegate() override {
    return &test_application_;
  }

  void EmbedApplicationWithURL(const std::string& url) {
    ServiceProviderPtr service_provider;
    BindToProxy(new ServiceProviderImpl, &service_provider);
    window_manager_->Embed(
        url, MakeRequest<ServiceProvider>(service_provider.PassMessagePipe()));

    base::RunLoop run_loop;
    test_application_.set_embed_callback(run_loop.QuitClosure());
    run_loop.Run();
  }

  WindowManagerPtr window_manager_;
  TestApplication test_application_;

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(WindowManagerApplicationTest);
};

TEST_F(WindowManagerApplicationTest, Embed) {
  EXPECT_EQ(nullptr, test_application_.root());
  EmbedApplicationWithURL("mojo:window_manager_apptests");
  EXPECT_NE(nullptr, test_application_.root());
}

// TODO(msw): Write tests exercising other WindowManager functionality.

}  // namespace
}  // namespace mojo
