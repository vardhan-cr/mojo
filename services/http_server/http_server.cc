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
#include "base/format_macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/stringprintf.h"
#include "mojo/common/weak_binding_set.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_runner.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/public/cpp/bindings/error_handler.h"
#include "mojo/public/cpp/environment/async_waiter.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/services/network/public/interfaces/network_service.mojom.h"
#include "services/http_server/http_request_parser.h"
#include "services/http_server/public/http_request.mojom.h"
#include "services/http_server/public/http_response.mojom.h"
#include "services/http_server/public/http_server.mojom.h"
#include "services/http_server/public/http_server_util.h"
#include "third_party/re2/re2/re2.h"

using mojo::AsyncWaiter;
using mojo::ScopedDataPipeConsumerHandle;
using mojo::ScopedDataPipeProducerHandle;
using mojo::TCPConnectedSocketPtr;

namespace http_server {

class Connection;

typedef base::Callback<void(HttpRequestPtr, Connection*)> HandleRequestCallback;

const char* GetHttpReasonPhrase(uint32_t code_in) {
  switch (code_in) {
#define HTTP_STATUS(label, code, reason) case code: return reason;
#include "net/http/http_status_code_list.h"
#undef HTTP_STATUS

    default:
      NOTREACHED() << "unknown HTTP status code " << code_in;
  }

  return "";
}

// Represents one connection to a client. This connection will manage its own
// lifetime and will delete itself when the connection is closed.
class Connection {
 public:
  // Callback called when a request is parsed. Response should be sent
  // using Connection::SendResponse() on the |connection| argument.
  typedef base::Callback<void(Connection*, HttpRequestPtr)> Callback;

  Connection(TCPConnectedSocketPtr conn,
             ScopedDataPipeProducerHandle sender,
             ScopedDataPipeConsumerHandle receiver,
             const Callback& callback)
      : connection_(conn.Pass()),
        sender_(sender.Pass()),
        receiver_(receiver.Pass()),
        request_waiter_(receiver_.get(), MOJO_HANDLE_SIGNAL_READABLE,
                        base::Bind(&Connection::OnRequestDataReady,
                                   base::Unretained(this))),
        content_length_(0),
        handle_request_callback_(callback),
        response_offset_(0) {
  }

  ~Connection() {
  }

  void SendResponse(HttpResponsePtr response) {
    std::string http_reason_phrase(GetHttpReasonPhrase(response->status_code));

    // TODO(mtomasz): For http/1.0 requests, send http/1.0.
    base::StringAppendF(&response_,
                        "HTTP/1.1 %d %s\r\n",
                        response->status_code,
                        http_reason_phrase.c_str());
    base::StringAppendF(&response_, "Connection: close\r\n");

    content_length_ = response->content_length;
    if (content_length_) {
      base::StringAppendF(&response_,
                          "Content-Length: %" PRIuS "\r\n",
                          static_cast<size_t>(content_length_));
    }
    base::StringAppendF(&response_,
                        "Content-Type: %s\r\n",
                        response->content_type.data());
    for (auto it = response->custom_headers.begin();
         it != response->custom_headers.end(); ++it) {
      const std::string& header_name = it.GetValue();
      const std::string& header_value = it.GetKey();
      DCHECK(header_value.find_first_of("\n\r") == std::string::npos) <<
          "Malformed header value.";
      base::StringAppendF(&response_,
                          "%s: %s\r\n",
                          header_name.c_str(),
                          header_value.c_str());
    }
    base::StringAppendF(&response_, "\r\n");

    content_ = response->body.Pass();
    WriteMore();
  }

 private:
  // Called when we have more data available from the request.
  void OnRequestDataReady(MojoResult result) {
    uint32_t num_bytes = 0;
    result = ReadDataRaw(receiver_.get(), NULL, &num_bytes,
                         MOJO_READ_DATA_FLAG_QUERY);
    if (!num_bytes)
      return;

    scoped_ptr<uint8_t[]> buffer(new uint8_t[num_bytes]);
    result = ReadDataRaw(receiver_.get(), buffer.get(), &num_bytes,
                         MOJO_READ_DATA_FLAG_ALL_OR_NONE);

    request_parser_.ProcessChunk(reinterpret_cast<char*>(buffer.get()));
    if (request_parser_.ParseRequest() == HttpRequestParser::ACCEPTED) {
      handle_request_callback_.Run(this, request_parser_.GetRequest());
    }
  }

  void WriteMore() {
    uint32_t response_bytes_available =
        static_cast<uint32_t>(response_.size() - response_offset_);
    if (response_bytes_available) {
      MojoResult result = WriteDataRaw(
          sender_.get(), &response_[response_offset_],
          &response_bytes_available, 0);
      if (result == MOJO_RESULT_SHOULD_WAIT) {
        sender_waiter_.reset(new AsyncWaiter(
            sender_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
            base::Bind(&Connection::OnSenderReady, base::Unretained(this))));
        return;
      } else if (result != MOJO_RESULT_OK) {
        LOG(ERROR) << "Error writing to pipe " << result;
        delete this;
        return;
      }

      response_offset_ += response_bytes_available;
    }

    if (response_offset_ != response_.size()) {
      // We have more data left in response_. Write more asynchronously.
      sender_waiter_.reset(new AsyncWaiter(
          sender_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
          base::Bind(&Connection::OnSenderReady, base::Unretained(this))));
      return;
    }

    // response_ is all sent, and there's no more data so we're done.
    if (!content_length_) {
      delete this;
      return;
    }

    // Copy data from the handler's pipe to response_.
    const uint32_t kMaxChunkSize = 1024 * 1024;

    uint32_t num_bytes_available = 0;
    MojoResult result = ReadDataRaw(content_.get(), NULL,
                                    &num_bytes_available,
                                    MOJO_READ_DATA_FLAG_QUERY);
    if (result == MOJO_RESULT_SHOULD_WAIT) {
      // Producer isn't ready yet. Wait for it.
      response_receiver_waiter_.reset(new AsyncWaiter(
          content_.get(), MOJO_HANDLE_SIGNAL_READABLE,
          base::Bind(&Connection::OnResponseDataReady,
                     base::Unretained(this))));
      return;
    }

    DCHECK_EQ(result, MOJO_RESULT_OK);
    num_bytes_available = std::min(num_bytes_available, kMaxChunkSize);

    response_.resize(num_bytes_available);
    response_offset_ = 0;
    content_length_ -= num_bytes_available;

    result = ReadDataRaw(content_.get(), &response_[0],
                         &num_bytes_available,
                         MOJO_READ_DATA_FLAG_ALL_OR_NONE);
    DCHECK_EQ(result, MOJO_RESULT_OK);
    sender_waiter_.reset(new AsyncWaiter(
        sender_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
        base::Bind(&Connection::OnSenderReady, base::Unretained(this))));
  }

  void OnResponseDataReady(MojoResult result) {
    DCHECK_EQ(result, MOJO_RESULT_OK);
    WriteMore();
  }

  void OnSenderReady(MojoResult result) {
    DCHECK_EQ(result, MOJO_RESULT_OK);
    WriteMore();
  }

  TCPConnectedSocketPtr connection_;
  ScopedDataPipeProducerHandle sender_;
  ScopedDataPipeConsumerHandle receiver_;

  // Used to wait for the request data.
  AsyncWaiter request_waiter_;

  int content_length_;
  ScopedDataPipeConsumerHandle content_;

  // Used to wait for the response data to send.
  scoped_ptr<AsyncWaiter> response_receiver_waiter_;

  // Used to wait for the sender to be ready to accept more data.
  scoped_ptr<AsyncWaiter> sender_waiter_;

  HttpRequestParser request_parser_;

  // Callback to run once all of the request has been read.
  const Callback handle_request_callback_;

  // Contains response data to write to the pipe. Initially it is the headers,
  // and then when they're written it contains chunks of the body.
  std::string response_;
  size_t response_offset_;
};

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
        std::remove_if(handlers_.begin(),
                       handlers_.end(),
                       [](Handler* h) {
                         return h->http_handler.encountered_error();
                       }));
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
