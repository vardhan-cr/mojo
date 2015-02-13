// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/http_server/http_server_factory_impl.h"

#include "base/stl_util.h"
#include "services/http_server/http_server_impl.h"
#include "services/http_server/public/http_server.mojom.h"

namespace http_server {

HttpServerFactoryImpl::HttpServerFactoryImpl(mojo::ApplicationImpl* app) {
  app_ = app;
}

HttpServerFactoryImpl::~HttpServerFactoryImpl() {
  // Free the http servers.
  STLDeleteContainerPairSecondPointers(port_indicated_servers_.begin(),
                                       port_indicated_servers_.end());
  STLDeleteContainerPointers(port_any_servers_.begin(),
                             port_any_servers_.end());
}

void HttpServerFactoryImpl::AddBinding(
    mojo::InterfaceRequest<HttpServerFactory> request) {
  bindings_.AddBinding(this, request.Pass());
}

void HttpServerFactoryImpl::DeleteServer(HttpServerImpl* server,
                                         uint16_t requested_port) {
  if (requested_port) {
    DCHECK(port_indicated_servers_.count(requested_port));
    DCHECK_EQ(server, port_indicated_servers_[requested_port]);

    delete server;
    port_indicated_servers_.erase(requested_port);
  } else {
    DCHECK(port_any_servers_.count(server));

    delete server;
    port_any_servers_.erase(server);
  }
}

void HttpServerFactoryImpl::CreateHttpServer(
    mojo::InterfaceRequest<HttpServer> server_request,
    uint16_t port) {
  if (port) {
    if (!port_indicated_servers_.count(port)) {
      port_indicated_servers_[port] = new HttpServerImpl(app_, this, port);
    }
    port_indicated_servers_[port]->AddBinding(server_request.Pass());
  } else {
    HttpServerImpl* server = new HttpServerImpl(app_, this, port);
    server->AddBinding(server_request.Pass());
    port_any_servers_.insert(server);
  }
}

}  // namespace http_server
