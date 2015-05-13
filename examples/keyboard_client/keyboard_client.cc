// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/services/keyboard/public/interfaces/keyboard.mojom.h"
#include "mojo/services/view_manager/public/cpp/view.h"
#include "mojo/services/view_manager/public/cpp/view_manager.h"
#include "mojo/services/view_manager/public/cpp/view_manager_client_factory.h"
#include "mojo/services/view_manager/public/cpp/view_manager_delegate.h"

namespace examples {

class KeyboardDelegate : public mojo::ApplicationDelegate,
                         public keyboard::KeyboardClient,
                         public mojo::ViewManagerDelegate {
 public:
  KeyboardDelegate() : binding_(this) {}

  ~KeyboardDelegate() override {}

  // mojo::ApplicationDelegate implementation.
  void Initialize(mojo::ApplicationImpl* app) override {
    view_manager_client_factory_.reset(
        new mojo::ViewManagerClientFactory(app->shell(), this));
  }

  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService(view_manager_client_factory_.get());
    return true;
  }

  // mojo::ViewManagerDelegate implementation.
  void OnEmbed(mojo::View* root,
               mojo::InterfaceRequest<mojo::ServiceProvider> services,
               mojo::ServiceProviderPtr exposed_services) override {
    mojo::View* content = root->view_manager()->CreateView();
    content->SetBounds(root->bounds());
    root->AddChild(content);
    content->SetVisible(true);

    mojo::ServiceProviderPtr keyboard_sp;
    content->Embed("mojo:keyboard_native", GetProxy(&keyboard_sp), nullptr);
    keyboard_sp->ConnectToService(keyboard::KeyboardService::Name_,
                                  GetProxy(&keyboard_).PassMessagePipe());

    keyboard_->ShowByRequest();
    keyboard_->Hide();

    keyboard::KeyboardClientPtr keyboard_client;
    auto request = mojo::GetProxy(&keyboard_client);
    binding_.Bind(request.Pass());
    keyboard_->Show(keyboard_client.Pass());
  }

  void OnViewManagerDisconnected(mojo::ViewManager* view_manager) override {
    base::MessageLoop::current()->Quit();
  }

  // keyboard::KeyboardClient implementation.
  void CommitCompletion(keyboard::CompletionDataPtr completion) override {}

  void CommitCorrection(keyboard::CorrectionDataPtr correction) override {}

  void CommitText(const mojo::String& text,
                  int32_t new_cursor_position) override {}

  void DeleteSurroundingText(int32_t before_length,
                             int32_t after_length) override {}

  void SetComposingRegion(int32_t start, int32_t end) override {}

  void SetComposingText(const mojo::String& text,
                        int32_t new_cursor_position) override {}

  void SetSelection(int32_t start, int32_t end) override {}

 private:
  mojo::Binding<keyboard::KeyboardClient> binding_;
  keyboard::KeyboardServicePtr keyboard_;
  scoped_ptr<mojo::ViewManagerClientFactory> view_manager_client_factory_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardDelegate);
};

}  // namespace examples

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new examples::KeyboardDelegate);
  return runner.Run(application_request);
}
