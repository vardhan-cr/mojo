// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#if defined(OS_WIN)
#include <winsock2.h>
#elif defined(OS_POSIX)
#include <arpa/inet.h>
#endif

#include <algorithm>

#include "base/bind.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/memory/weak_ptr.h"
#include "mojo/common/weak_binding_set.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_runner.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/public/cpp/bindings/error_handler.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/services/network/public/interfaces/network_service.mojom.h"
#include "services/http_server/connection.h"
#include "services/http_server/public/http_request.mojom.h"
#include "services/http_server/public/http_response.mojom.h"
#include "services/http_server/public/http_server.mojom.h"
#include "services/http_server/public/http_server_util.h"
#include "third_party/re2/re2/re2.h"

using mojo::ScopedDataPipeConsumerHandle;
using mojo::ScopedDataPipeProducerHandle;
using mojo::TCPConnectedSocketPtr;

namespace http_server {

class HttpServerApp : public HttpServer,
                      public mojo::ApplicationDelegate,
                      public mojo::ErrorHandler,
                      public mojo::InterfaceFactory<HttpServer> {
 public:
  HttpServerApp() : weak_ptr_factory_(this) {}
  virtual void Initialize(mojo::ApplicationImpl* app) override {
    app->ConnectToService("mojo:network_service", &network_service_);
    Start();
  }

  // HttpServer
  void SetHandler(const mojo::String& path,
                  HttpHandlerPtr http_handler,
                  const mojo::Callback<void(bool)>& callback) override {
    for (const auto& handler : handlers_) {
      if (handler->pattern->pattern() == path)
        callback.Run(false);
    }

    http_handler.set_error_handler(this);
    handlers_.push_back(new Handler(path, http_handler.Pass()));
    callback.Run(true);
  }

 private:
  // ErrorHandler:
  void OnConnectionError() override {
    handlers_.erase(
        std::remove_if(handlers_.begin(), handlers_.end(), [](Handler* h) {
      return h->http_handler.encountered_error();
    }), handlers_.end());
  }

  // ApplicationDelegate:
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService(this);
    return true;
  }

  // InterfaceFactory<HttpServerService>:
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<HttpServer> request) override {
    http_server_bindings_.AddBinding(this, request.Pass());
  }

  void OnSocketBound(mojo::NetworkErrorPtr err,
                     mojo::NetAddressPtr bound_address) {
    if (err->code != 0) {
      printf("Bound err = %d\n", err->code);
      return;
    }

    printf("Got address %d.%d.%d.%d:%d\n",
           (int)bound_address->ipv4->addr[0],
           (int)bound_address->ipv4->addr[1],
           (int)bound_address->ipv4->addr[2],
           (int)bound_address->ipv4->addr[3],
           (int)bound_address->ipv4->port);
  }

  void OnSocketListening(mojo::NetworkErrorPtr err) {
    if (err->code != 0) {
      printf("Listen err = %d\n", err->code);
      return;
    }
    printf("Waiting for incoming connections...\n");
  }

  void OnConnectionAccepted(mojo::NetworkErrorPtr err,
                            mojo::NetAddressPtr remote_address) {
    if (err->code != 0) {
      printf("Accepted socket error = %d\n", err->code);
      return;
    }

    new Connection(pending_connected_socket_.Pass(),
                   pending_send_handle_.Pass(),
                   pending_receive_handle_.Pass(),
                   base::Bind(&HttpServerApp::HandleRequest,
                              weak_ptr_factory_.GetWeakPtr()));

    // Ready for another connection.
    WaitForNextConnection();
  }

  void WaitForNextConnection() {
    // Need two pipes (one for each direction).
    ScopedDataPipeConsumerHandle send_consumer_handle;
    MojoResult result = CreateDataPipe(
        nullptr, &pending_send_handle_, &send_consumer_handle);
    assert(result == MOJO_RESULT_OK);

    mojo::ScopedDataPipeProducerHandle receive_producer_handle;
    result = CreateDataPipe(
        nullptr, &receive_producer_handle, &pending_receive_handle_);
    assert(result == MOJO_RESULT_OK);
    MOJO_ALLOW_UNUSED_LOCAL(result);

    server_socket_->Accept(send_consumer_handle.Pass(),
                           receive_producer_handle.Pass(),
                           GetProxy(&pending_connected_socket_),
                           base::Bind(&HttpServerApp::OnConnectionAccepted,
                                      base::Unretained(this)));
  }

  void Start() {
    mojo::NetAddressPtr net_address(mojo::NetAddress::New());
    net_address->family = mojo::NET_ADDRESS_FAMILY_IPV4;
    net_address->ipv4 = mojo::NetAddressIPv4::New();
    net_address->ipv4->addr.resize(4);
    net_address->ipv4->addr[0] = 0;
    net_address->ipv4->addr[1] = 0;
    net_address->ipv4->addr[2] = 0;
    net_address->ipv4->addr[3] = 0;
    net_address->ipv4->port = 80;

    // Note that we can start using the proxies right away even thought the
    // callbacks have not been called yet. If a previous step fails, they'll
    // all fail.
    network_service_->CreateTCPBoundSocket(
        net_address.Pass(),
        GetProxy(&bound_socket_),
        base::Bind(&HttpServerApp::OnSocketBound, base::Unretained(this)));
    bound_socket_->StartListening(GetProxy(
        &server_socket_),
        base::Bind(&HttpServerApp::OnSocketListening, base::Unretained(this)));
    WaitForNextConnection();
  }

  void HandleRequest(Connection* connection, HttpRequestPtr request) {
    for (auto& handler : handlers_) {
      if (RE2::FullMatch(request->relative_url.data(), *handler->pattern)) {
        handler->http_handler->HandleRequest(
            request.Pass(),
            base::Bind(&HttpServerApp::OnResponse,
                       base::Unretained(this),
                       connection));
        return;
      }
    }

    connection->SendResponse(
        CreateHttpResponse(404, "No registered handler\n"));
  }

  void OnResponse(Connection* connection, HttpResponsePtr response) {
    connection->SendResponse(response.Pass());
  }

  struct Handler {
    Handler(const std::string& pattern,
            HttpHandlerPtr http_handler)
        : pattern(new RE2(pattern.c_str())), http_handler(http_handler.Pass()) {
    }
    scoped_ptr<RE2> pattern;
    HttpHandlerPtr http_handler;
   private:
    DISALLOW_COPY_AND_ASSIGN(Handler);
  };

  base::WeakPtrFactory<HttpServerApp> weak_ptr_factory_;
  mojo::WeakBindingSet<HttpServer> http_server_bindings_;

  mojo::NetworkServicePtr network_service_;
  mojo::TCPBoundSocketPtr bound_socket_;
  mojo::TCPServerSocketPtr server_socket_;

  ScopedDataPipeProducerHandle pending_send_handle_;
  ScopedDataPipeConsumerHandle pending_receive_handle_;
  TCPConnectedSocketPtr pending_connected_socket_;

  ScopedVector<Handler> handlers_;
};

}  // namespace http_server

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunner runner(new http_server::HttpServerApp);
  return runner.Run(shell_handle);
}
