// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/macros.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/common/binding_set.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/services/notifications/public/interfaces/notifications.mojom.h"

namespace examples {

static const base::TimeDelta kDefaultMessageDelay =
    base::TimeDelta::FromMilliseconds(3000);

class NotificationGeneratorDelegate : public mojo::ApplicationDelegate,
                                      public notifications::NotificationClient {
 public:
  NotificationGeneratorDelegate() {}

  ~NotificationGeneratorDelegate() override {}

  // mojo::ApplicationDelegate implementation.
  void Initialize(mojo::ApplicationImpl* app) override {
    app->ConnectToService("mojo:notifications", &notification_service_);
    PostFirstNotification();
  }

  // notifications::NotificationClient implementation.
  void OnSelected() override {
    PostNotification("You selected a notification!",
                     "Have you dismissed one yet?", &select_notification_);
  }

  void OnDismissed() override {
    PostNotification("You dismissed a notification!",
                     "Have you selected one yet?", &dismissed_notification_);
  }

  void PostFirstNotification() {
    PostNotification("First notification", "Next: Second will be created",
                     &first_notification_);
    PostDelayed(
        base::Bind(&NotificationGeneratorDelegate::PostSecondNotification,
                   base::Unretained(this)));
  }

  void PostSecondNotification() {
    PostNotification("Second notification", "Next: First will be updated",
                     &second_notification_);
    PostDelayed(
        base::Bind(&NotificationGeneratorDelegate::UpdateFirstNotification,
                   base::Unretained(this)));
  }

  void PostNotification(const char* title,
                        const char* text,
                        notifications::NotificationPtr* notification) {
    notifications::NotificationClientPtr notification_client;
    auto request = mojo::GetProxy(&notification_client);
    client_bindings_.AddBinding(this, request.Pass());
    notification_service_->Post(CreateNotificationData(title, text).Pass(),
                                notification_client.Pass(),
                                GetProxy(notification));
  }

  void UpdateFirstNotification() {
    first_notification_->Update(
        CreateNotificationData("First notification updated",
                               "Next: both cancelled; repeat").Pass());
    PostDelayed(
        base::Bind(&NotificationGeneratorDelegate::CancelSecondNotification,
                   base::Unretained(this)));
  }

  void CancelSecondNotification() {
    second_notification_->Cancel();
    PostDelayed(
        base::Bind(&NotificationGeneratorDelegate::CancelFirstNotification,
                   base::Unretained(this)));
  }

  void CancelFirstNotification() {
    first_notification_->Cancel();
    PostDelayed(
        base::Bind(&NotificationGeneratorDelegate::PostFirstNotification,
                   base::Unretained(this)));
  }

  notifications::NotificationDataPtr CreateNotificationData(const char* title,
                                                            const char* text) {
    notifications::NotificationDataPtr notification_data =
        notifications::NotificationData::New();
    notification_data->title = mojo::String(title);
    notification_data->text = mojo::String(text);
    return notification_data;
  }

  void PostDelayed(base::Closure closure) {
    base::MessageLoop::current()->PostDelayedTask(FROM_HERE, closure,
                                                  kDefaultMessageDelay);
  }

 private:
  notifications::NotificationServicePtr notification_service_;
  mojo::BindingSet<notifications::NotificationClient> client_bindings_;
  notifications::NotificationPtr first_notification_;
  notifications::NotificationPtr second_notification_;
  notifications::NotificationPtr dismissed_notification_;
  notifications::NotificationPtr select_notification_;

  DISALLOW_COPY_AND_ASSIGN(NotificationGeneratorDelegate);
};

}  // namespace examples

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(
      new examples::NotificationGeneratorDelegate);
  return runner.Run(application_request);
}
