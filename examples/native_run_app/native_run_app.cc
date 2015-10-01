// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a terminal client (i.e., a "raw" |mojo.terminal.Terminal| -- e.g.,
// moterm -- can be asked to talk to this) that prompts the user for a native
// (Linux) binary to run and then does so (via mojo:native_support).
//
// E.g., first run mojo:moterm_example_app (embedded by a window manager). Then,
// at the prompt, enter "mojo:native_run_app". At the next prompt, enter "bash"
// (or "echo hello mojo").
//
// TODO(vtl): Maybe it should optionally be able to extract the binary path (and
// args) from the connection URL?

#include <string.h>

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_split.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/files/public/interfaces/files.mojom.h"
#include "mojo/services/files/public/interfaces/ioctl.mojom.h"
#include "mojo/services/files/public/interfaces/ioctl_terminal.mojom.h"
#include "mojo/services/files/public/interfaces/types.mojom.h"
#include "mojo/services/native_support/public/interfaces/process.mojom.h"
#include "mojo/services/terminal/public/interfaces/terminal_client.mojom.h"

using mojo::terminal::TerminalClient;

class TerminalConnection {
 public:
  explicit TerminalConnection(mojo::files::FilePtr terminal,
                              native_support::Process* native_support_process)
      : terminal_(terminal.Pass()),
        native_support_process_(native_support_process) {
    terminal_.set_connection_error_handler([this]() { delete this; });
    Start();
  }
  ~TerminalConnection() {}

 private:
  void Write(const char* s, mojo::files::File::WriteCallback callback) {
    size_t length = strlen(s);
    mojo::Array<uint8_t> a(length);
    memcpy(&a[0], s, length);
    terminal_->Write(a.Pass(), 0, mojo::files::WHENCE_FROM_CURRENT, callback);
  }

  void Start() {
    // TODO(vtl): Check canonical mode (via ioctl) first (or before |Read()|).

    const char kPrompt[] = "\x1b[0mNative program to run?\n>>> ";
    Write(kPrompt, [this](mojo::files::Error error, uint32_t) {
      this->DidWritePrompt(error);
    });
  }
  void DidWritePrompt(mojo::files::Error error) {
    if (error != mojo::files::ERROR_OK) {
      LOG(ERROR) << "Write() error: " << error;
      delete this;
      return;
    }

    terminal_->Read(
        1000, 0, mojo::files::WHENCE_FROM_CURRENT,
        [this](mojo::files::Error error, mojo::Array<uint8_t> bytes_read) {
          this->DidReadFromPrompt(error, bytes_read.Pass());
        });
  }
  void DidReadFromPrompt(mojo::files::Error error,
                         mojo::Array<uint8_t> bytes_read) {
    if (error != mojo::files::ERROR_OK || !bytes_read.size()) {
      LOG(ERROR) << "Read() error: " << error;
      delete this;
      return;
    }

    std::string input(reinterpret_cast<const char*>(&bytes_read[0]),
                      bytes_read.size());
    command_line_.clear();
    base::SplitStringAlongWhitespace(input, &command_line_);

    if (command_line_.empty()) {
      Start();
      return;
    }

    // Set the terminal to noncanonical mode. Do so by getting the settings,
    // flipping the flag, and setting them.
    // TODO(vtl): Should it do other things?
    // TODO(vtl): I wonder if these ioctls shouldn't be done by the
    // |SpawnWithTerminal()| implementation instead. Hmmm.
    mojo::Array<uint32_t> in_values = mojo::Array<uint32_t>::New(1);
    in_values[0] = mojo::files::kIoctlTerminalGetSettings;
    terminal_->Ioctl(
        mojo::files::kIoctlTerminal, in_values.Pass(),
        [this](mojo::files::Error error, mojo::Array<uint32_t> out_values) {
          this->DidGetTerminalSettings(error, out_values.Pass());
        });
  }
  void DidGetTerminalSettings(mojo::files::Error error,
                              mojo::Array<uint32_t> out_values) {
    if (error != mojo::files::ERROR_OK || out_values.size() < 6) {
      LOG(ERROR) << "Ioctl() (terminal get settings) error: " << error;
      delete this;
      return;
    }

    const size_t kBaseFieldCount =
        mojo::files::kIoctlTerminalTermiosBaseFieldCount;
    const uint32_t kLFlagIdx = mojo::files::kIoctlTerminalTermiosLFlagIndex;
    const uint32_t kLFlagICANON = mojo::files::kIoctlTerminalTermiosLFlagICANON;

    auto in_values = mojo::Array<uint32_t>::New(1 + kBaseFieldCount);
    in_values[0] = mojo::files::kIoctlTerminalSetSettings;
    for (size_t i = 0; i < kBaseFieldCount; i++)
      in_values[1 + i] = out_values[i];
    // Just turn off ICANON, which is in "lflag".
    in_values[1 + kLFlagIdx] &= ~kLFlagICANON;
    terminal_->Ioctl(
        mojo::files::kIoctlTerminal, in_values.Pass(),
        [this](mojo::files::Error error, mojo::Array<uint32_t> out_values) {
          this->DidSetTerminalSettings(error, out_values.Pass());
        });
  }
  void DidSetTerminalSettings(mojo::files::Error error,
                              mojo::Array<uint32_t> out_values) {
    if (error != mojo::files::ERROR_OK) {
      LOG(ERROR) << "Ioctl() (terminal set settings) error: " << error;
      delete this;
      return;
    }

    // Now, we can spawn.
    mojo::String path(command_line_[0]);
    mojo::Array<mojo::String> argv;
    for (const auto& arg : command_line_)
      argv.push_back(arg);

    // TODO(vtl): If the |InterfacePtr| underlying |native_support_process_|
    // encounters an error, then we're sort of dead in the water.
    native_support_process_->SpawnWithTerminal(
        path, argv.Pass(), mojo::Array<mojo::String>(), terminal_.Pass(),
        GetProxy(&process_controller_), [this](mojo::files::Error error) {
          this->DidSpawnWithTerminal(error);
        });
    process_controller_.set_connection_error_handler([this]() { delete this; });
  }
  void DidSpawnWithTerminal(mojo::files::Error error) {
    if (error != mojo::files::ERROR_OK) {
      LOG(ERROR) << "SpawnWithTerminal() error: " << error;
      delete this;
      return;
    }
    process_controller_->Wait(
        [this](mojo::files::Error error, int32_t exit_status) {
          this->DidWait(error, exit_status);
        });
  }
  void DidWait(mojo::files::Error error, int32_t exit_status) {
    if (error != mojo::files::ERROR_OK)
      LOG(ERROR) << "Wait() error: " << error;
    else if (exit_status != 0)  // |exit_status| only valid if OK.
      LOG(ERROR) << "Process exit status: " << exit_status;

    // We're done, regardless.
    delete this;
  }

  mojo::files::FilePtr terminal_;
  native_support::Process* native_support_process_;
  native_support::ProcessControllerPtr process_controller_;

  std::vector<std::string> command_line_;

  DISALLOW_COPY_AND_ASSIGN(TerminalConnection);
};

class TerminalClientImpl : public TerminalClient {
 public:
  TerminalClientImpl(mojo::InterfaceRequest<TerminalClient> request,
                     native_support::Process* native_support_process)
      : binding_(this, request.Pass()),
        native_support_process_(native_support_process) {}
  ~TerminalClientImpl() override {}

  // |TerminalClient| implementation:
  void ConnectToTerminal(mojo::files::FilePtr terminal) override {
    if (terminal) {
      // Owns itself.
      new TerminalConnection(terminal.Pass(), native_support_process_);
    } else {
      LOG(ERROR) << "No terminal";
    }
  }

 private:
  mojo::StrongBinding<TerminalClient> binding_;
  native_support::Process* native_support_process_;

  DISALLOW_COPY_AND_ASSIGN(TerminalClientImpl);
};

class NativeRunApp : public mojo::ApplicationDelegate,
                     public mojo::InterfaceFactory<TerminalClient> {
 public:
  NativeRunApp() : application_impl_(nullptr) {}
  ~NativeRunApp() override {}

 private:
  // |mojo::ApplicationDelegate|:
  void Initialize(mojo::ApplicationImpl* application_impl) override {
    DCHECK(!application_impl_);
    application_impl_ = application_impl;
    application_impl_->ConnectToService("mojo:native_support",
                                        &native_support_process_);
  }

  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService<TerminalClient>(this);
    return true;
  }

  // |InterfaceFactory<TerminalClient>| implementation:
  void Create(mojo::ApplicationConnection* /*connection*/,
              mojo::InterfaceRequest<TerminalClient> request) override {
    new TerminalClientImpl(request.Pass(), native_support_process_.get());
  }

  mojo::ApplicationImpl* application_impl_;
  native_support::ProcessPtr native_support_process_;

  DISALLOW_COPY_AND_ASSIGN(NativeRunApp);
};

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new NativeRunApp());
  return runner.Run(application_request);
}
