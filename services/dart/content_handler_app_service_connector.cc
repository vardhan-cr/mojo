// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/dart/content_handler_app_service_connector.h"

#include "base/location.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/services/network/public/interfaces/network_service.mojom.h"

namespace dart {

// This callback runs on the Dart content handler message loop thread. Bound
// to |this| by a weak pointer.
template<typename Interface>
void ContentHandlerAppServiceConnector::Connect(
    std::string application_name,
    mojo::InterfaceRequest<Interface> interface_request) {
  mojo::ApplicationConnection* application_connection =
      content_handler_app_->ConnectToApplication(application_name);
  if (!application_connection)
    return;
  mojo::ServiceProvider* sp = application_connection->GetServiceProvider();
  if (!sp)
    return;
  sp->ConnectToService(Interface::Name_,
                       interface_request.PassMessagePipe().Pass());
}

ContentHandlerAppServiceConnector::ContentHandlerAppServiceConnector(
    mojo::ApplicationImpl* content_handler_app)
        : runner_(base::MessageLoop::current()->task_runner()),
          content_handler_app_(content_handler_app),
          weak_ptr_factory_(this) {
  CHECK(content_handler_app != nullptr);
  CHECK(runner_.get() != nullptr);
  CHECK(runner_.get()->BelongsToCurrentThread());
}

ContentHandlerAppServiceConnector::~ContentHandlerAppServiceConnector() {
}

MojoHandle ContentHandlerAppServiceConnector::ConnectToService(
    ServiceId service_id) {
  switch (service_id) {
    case mojo::dart::DartControllerServiceConnector::kNetworkServiceId: {
      std::string application_name = "mojo:network_service";
      // Construct proxy.
      mojo::NetworkServicePtr interface_ptr;
      runner_->PostTask(FROM_HERE, base::Bind(
          &ContentHandlerAppServiceConnector::Connect<mojo::NetworkService>,
          weak_ptr_factory_.GetWeakPtr(),
          application_name,
          base::Passed(GetProxy(&interface_ptr))));
      // Return proxy end of pipe to caller.
      return interface_ptr.PassMessagePipe().release().value();
    }
    break;
    default:
      return MOJO_HANDLE_INVALID;
    break;
  }
}

}  // namespace dart
