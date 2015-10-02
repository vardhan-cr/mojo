// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a simple example application (with an embeddable view), which embeds
// the Moterm view, uses it to prompt the user, etc.

#include <string.h>

#include <algorithm>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/bindings/array.h"
#include "mojo/public/interfaces/application/service_provider.mojom.h"
#include "mojo/public/interfaces/application/shell.mojom.h"
#include "mojo/services/files/public/interfaces/file.mojom.h"
#include "mojo/services/files/public/interfaces/types.mojom.h"
#include "mojo/services/terminal/public/interfaces/terminal.mojom.h"
#include "mojo/services/terminal/public/interfaces/terminal_client.mojom.h"
#include "mojo/services/view_manager/public/cpp/view.h"
#include "mojo/services/view_manager/public/cpp/view_manager.h"
#include "mojo/services/view_manager/public/cpp/view_manager_client_factory.h"
#include "mojo/services/view_manager/public/cpp/view_manager_delegate.h"
#include "mojo/services/view_manager/public/cpp/view_observer.h"

// Kind of like |fputs()| (doesn't wait for result).
void Fputs(mojo::files::File* file, const char* s) {
  size_t length = strlen(s);
  mojo::Array<uint8_t> a(length);
  memcpy(&a[0], s, length);

  file->Write(a.Pass(), 0, mojo::files::Whence::FROM_CURRENT,
              mojo::files::File::WriteCallback());
}

class MotermExampleAppView : public mojo::ViewObserver {
 public:
  explicit MotermExampleAppView(mojo::Shell* shell, mojo::View* view)
      : shell_(shell), view_(view), moterm_view_(), weak_factory_(this) {
    view_->AddObserver(this);

    moterm_view_ = view_->view_manager()->CreateView();
    view_->AddChild(moterm_view_);
    moterm_view_->SetBounds(view_->bounds());
    moterm_view_->SetVisible(true);
    mojo::ServiceProviderPtr moterm_sp;
    moterm_view_->Embed("mojo:moterm", GetProxy(&moterm_sp), nullptr);
    moterm_sp->ConnectToService(mojo::terminal::Terminal::Name_,
                                GetProxy(&moterm_terminal_).PassMessagePipe());
    Resize();
    StartPrompt(true);
  }

  ~MotermExampleAppView() override {}

 private:
  void Resize() {
    moterm_terminal_->SetSize(0, 0, false, [](mojo::files::Error error,
                                              uint32_t rows, uint32_t columns) {
      DCHECK_EQ(error, mojo::files::Error::OK);
      DVLOG(1) << "New size: " << rows << "x" << columns;
    });
  }

  void StartPrompt(bool first_time) {
    if (!moterm_file_) {
      moterm_terminal_->Connect(GetProxy(&moterm_file_), false,
                                [](mojo::files::Error error) {
                                  DCHECK_EQ(error, mojo::files::Error::OK);
                                });
    }

    if (first_time) {
      // Display some welcome text.
      Fputs(moterm_file_.get(),
            "Welcome to "
            "\x1b[1m\x1b[34mM\x1b[31mo\x1b[33mt\x1b[34me\x1b[32mr\x1b[31mm\n"
            "\n");
    }

    Fputs(moterm_file_.get(), "\x1b[0m\nWhere do you want to go today?\n:) ");
    moterm_file_->Read(1000, 0, mojo::files::Whence::FROM_CURRENT,
                       base::Bind(&MotermExampleAppView::OnInputFromPrompt,
                                  weak_factory_.GetWeakPtr()));
  }
  void OnInputFromPrompt(mojo::files::Error error,
                         mojo::Array<uint8_t> bytes_read) {
    if (error != mojo::files::Error::OK || !bytes_read) {
      // TODO(vtl): Handle errors?
      NOTIMPLEMENTED();
      return;
    }

    std::string dest_url;
    if (bytes_read.size() >= 1) {
      base::TrimWhitespaceASCII(
          std::string(reinterpret_cast<const char*>(&bytes_read[0]),
                      bytes_read.size()),
          base::TRIM_ALL, &dest_url);
    }

    if (dest_url.empty()) {
      Fputs(moterm_file_.get(), "\nError: no destination URL given\n");
      StartPrompt(false);
      return;
    }

    Fputs(moterm_file_.get(),
          base::StringPrintf("\nGoing to %s ...\n", dest_url.c_str()).c_str());
    moterm_file_.reset();

    mojo::ServiceProviderPtr dest_sp;
    shell_->ConnectToApplication(dest_url, GetProxy(&dest_sp), nullptr);
    mojo::terminal::TerminalClientPtr dest_terminal_client;
    mojo::ConnectToService(dest_sp.get(), &dest_terminal_client);
    moterm_terminal_->ConnectToClient(
        dest_terminal_client.Pass(), true,
        base::Bind(&MotermExampleAppView::OnDestinationDone,
                   weak_factory_.GetWeakPtr()));
  }
  void OnDestinationDone(mojo::files::Error error) {
    // We should always succeed. (It'll only fail on synchronous failures, which
    // only occur when it's busy.)
    DCHECK_EQ(error, mojo::files::Error::OK);
    StartPrompt(false);
  }

  // |ViewObserver|:
  void OnViewDestroyed(mojo::View* view) override {
    DCHECK(view == view_);
    delete this;
  }

  void OnViewBoundsChanged(mojo::View* view,
                           const mojo::Rect& old_bounds,
                           const mojo::Rect& new_bounds) override {
    DCHECK_EQ(view, view_);
    moterm_view_->SetBounds(view_->bounds());
    Resize();
  }

  mojo::Shell* const shell_;
  mojo::View* const view_;

  mojo::View* moterm_view_;
  mojo::terminal::TerminalPtr moterm_terminal_;
  // Valid while prompting.
  mojo::files::FilePtr moterm_file_;

  base::WeakPtrFactory<MotermExampleAppView> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MotermExampleAppView);
};

class MotermExampleApp : public mojo::ApplicationDelegate,
                         public mojo::ViewManagerDelegate {
 public:
  MotermExampleApp() : application_impl_() {}
  ~MotermExampleApp() override {}

 private:
  // |mojo::ApplicationDelegate|:
  void Initialize(mojo::ApplicationImpl* application_impl) override {
    DCHECK(!application_impl_);
    application_impl_ = application_impl;
    view_manager_client_factory_.reset(
        new mojo::ViewManagerClientFactory(application_impl_->shell(), this));
  }

  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService(view_manager_client_factory_.get());
    return true;
  }

  // |mojo::ViewManagerDelegate|:
  void OnEmbed(mojo::View* root,
               mojo::InterfaceRequest<mojo::ServiceProvider> services,
               mojo::ServiceProviderPtr exposed_services) override {
    new MotermExampleAppView(application_impl_->shell(), root);
  }

  void OnViewManagerDisconnected(mojo::ViewManager* view_manager) override {}

  mojo::ApplicationImpl* application_impl_;
  scoped_ptr<mojo::ViewManagerClientFactory> view_manager_client_factory_;

  DISALLOW_COPY_AND_ASSIGN(MotermExampleApp);
};

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new MotermExampleApp());
  return runner.Run(application_request);
}
