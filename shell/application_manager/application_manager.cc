// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/application_manager/application_manager.h"

#include <stdio.h>

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/stl_util.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/error_handler.h"
#include "mojo/public/interfaces/application/shell.mojom.h"
#include "mojo/services/content_handler/public/interfaces/content_handler.mojom.h"

namespace mojo {

namespace {
// Used by TestAPI.
bool has_created_instance = false;
}  // namespace


ApplicationManager::Delegate::~Delegate() {
}

void ApplicationManager::Delegate::OnApplicationError(const GURL& url) {
  LOG(ERROR) << "Communication error with application: " << url.spec();
}

GURL ApplicationManager::Delegate::ResolveURL(const GURL& url) {
  return url;
}

GURL ApplicationManager::Delegate::ResolveMappings(const GURL& url) {
  return url;
}

class ApplicationManager::ContentHandlerConnection : public ErrorHandler {
 public:
  ContentHandlerConnection(ApplicationManager* manager,
                           const GURL& content_handler_url)
      : manager_(manager), content_handler_url_(content_handler_url) {
    ServiceProviderPtr services;
    manager->ConnectToApplication(content_handler_url, GURL(),
                                  GetProxy(&services), nullptr);
    mojo::ConnectToService(services.get(), &content_handler_);
    content_handler_.set_error_handler(this);
  }

  ContentHandler* content_handler() { return content_handler_.get(); }

  GURL content_handler_url() { return content_handler_url_; }

 private:
  // ErrorHandler implementation:
  void OnConnectionError() override { manager_->OnContentHandlerError(this); }

  ApplicationManager* manager_;
  GURL content_handler_url_;
  ContentHandlerPtr content_handler_;

  DISALLOW_COPY_AND_ASSIGN(ContentHandlerConnection);
};

// static
ApplicationManager::TestAPI::TestAPI(ApplicationManager* manager)
    : manager_(manager) {
}

ApplicationManager::TestAPI::~TestAPI() {
}

bool ApplicationManager::TestAPI::HasCreatedInstance() {
  return has_created_instance;
}

bool ApplicationManager::TestAPI::HasFactoryForURL(const GURL& url) const {
  return manager_->url_to_shell_impl_.find(url) !=
         manager_->url_to_shell_impl_.end();
}

ApplicationManager::ApplicationManager(Delegate* delegate)
    : delegate_(delegate),
      weak_ptr_factory_(this) {
}

ApplicationManager::~ApplicationManager() {
  STLDeleteValues(&url_to_content_handler_);
  TerminateShellConnections();
  STLDeleteValues(&url_to_loader_);
  STLDeleteValues(&scheme_to_loader_);
}

void ApplicationManager::TerminateShellConnections() {
  STLDeleteValues(&url_to_shell_impl_);
}

void ApplicationManager::ConnectToApplication(
    const GURL& requested_url,
    const GURL& requestor_url,
    InterfaceRequest<ServiceProvider> services,
    ServiceProviderPtr exposed_services) {
  DCHECK(requested_url.is_valid());
  GURL mapped_url = delegate_->ResolveMappings(requested_url);

  // We check both the mapped and resolved urls for existing shell_impls because
  // external applications can be registered for the unresolved mojo:foo urls.
  ShellImpl* shell_impl = GetShellImpl(mapped_url);
  if (shell_impl) {
    ConnectToClient(shell_impl, mapped_url, requestor_url, services.Pass(),
                    exposed_services.Pass());
    return;
  }

  GURL resolved_url = delegate_->ResolveURL(mapped_url);
  shell_impl = GetShellImpl(resolved_url);
  if (shell_impl) {
    ConnectToClient(shell_impl, resolved_url, requestor_url, services.Pass(),
                    exposed_services.Pass());
    return;
  }

  ApplicationLoader* loader =
      GetLoaderForURL(mapped_url, DONT_INCLUDE_DEFAULT_LOADER);
  if (loader) {
    ConnectToApplicationImpl(requested_url, mapped_url, requestor_url,
                             services.Pass(), exposed_services.Pass(), loader);
    return;
  }

  loader = GetLoaderForURL(resolved_url, INCLUDE_DEFAULT_LOADER);
  if (loader) {
    ConnectToApplicationImpl(requested_url, resolved_url, requestor_url,
                             services.Pass(), exposed_services.Pass(), loader);
    return;
  }

  LOG(WARNING) << "Could not find loader to load application: "
               << requested_url.spec();
}

void ApplicationManager::ConnectToApplicationImpl(
    const GURL& requested_url,
    const GURL& resolved_url,
    const GURL& requestor_url,
    InterfaceRequest<ServiceProvider> services,
    ServiceProviderPtr exposed_services,
    ApplicationLoader* loader) {
  ApplicationPtr application;
  InterfaceRequest<Application> application_request = GetProxy(&application);
  ShellImpl* shell =
      new ShellImpl(application.Pass(), this, requested_url, resolved_url);
  url_to_shell_impl_[resolved_url] = shell;
  shell->InitializeApplication(GetArgsForURL(requested_url));

  loader->Load(this, resolved_url, application_request.Pass(),
               base::Bind(&ApplicationManager::LoadWithContentHandler,
                          weak_ptr_factory_.GetWeakPtr()));
  ConnectToClient(shell, resolved_url, requestor_url, services.Pass(),
                  exposed_services.Pass());
}

ShellImpl* ApplicationManager::GetShellImpl(const GURL& url) {
  const auto& shell_it = url_to_shell_impl_.find(url);
  if (shell_it != url_to_shell_impl_.end())
    return shell_it->second;
  return nullptr;
}

void ApplicationManager::ConnectToClient(
    ShellImpl* shell_impl,
    const GURL& url,
    const GURL& requestor_url,
    InterfaceRequest<ServiceProvider> services,
    ServiceProviderPtr exposed_services) {
  shell_impl->ConnectToClient(requestor_url, services.Pass(),
                              exposed_services.Pass());
}

void ApplicationManager::RegisterExternalApplication(
    const GURL& url,
    const std::vector<std::string>& args,
    ApplicationPtr application) {
  ShellImpl* shell_impl = new ShellImpl(application.Pass(), this, url, url);
  url_to_shell_impl_[url] = shell_impl;

  if (args.empty())
    shell_impl->InitializeApplication(GetArgsForURL(url));
  else
    shell_impl->InitializeApplication(Array<String>::From(args));
}

void ApplicationManager::LoadWithContentHandler(
    const GURL& content_handler_url,
    InterfaceRequest<Application> application_request,
    URLResponsePtr url_response) {
  ContentHandlerConnection* connection = NULL;
  URLToContentHandlerMap::iterator iter =
      url_to_content_handler_.find(content_handler_url);
  if (iter != url_to_content_handler_.end()) {
    connection = iter->second;
  } else {
    connection = new ContentHandlerConnection(this, content_handler_url);
    url_to_content_handler_[content_handler_url] = connection;
  }

  connection->content_handler()->StartApplication(application_request.Pass(),
                                                  url_response.Pass());
}

void ApplicationManager::SetLoaderForURL(scoped_ptr<ApplicationLoader> loader,
                                         const GURL& url) {
  URLToLoaderMap::iterator it = url_to_loader_.find(url);
  if (it != url_to_loader_.end())
    delete it->second;
  url_to_loader_[url] = loader.release();
}

void ApplicationManager::SetLoaderForScheme(
    scoped_ptr<ApplicationLoader> loader,
    const std::string& scheme) {
  SchemeToLoaderMap::iterator it = scheme_to_loader_.find(scheme);
  if (it != scheme_to_loader_.end())
    delete it->second;
  scheme_to_loader_[scheme] = loader.release();
}

void ApplicationManager::SetArgsForURL(const std::vector<std::string>& args,
                                       const GURL& url) {
  url_to_args_[url] = args;
}

ApplicationLoader* ApplicationManager::GetLoaderForURL(
    const GURL& url, IncludeDefaultLoader include_default_loader) {
  auto url_it = url_to_loader_.find(url);
  if (url_it != url_to_loader_.end())
    return url_it->second;
  auto scheme_it = scheme_to_loader_.find(url.scheme());
  if (scheme_it != scheme_to_loader_.end())
    return scheme_it->second;
  if (include_default_loader == INCLUDE_DEFAULT_LOADER)
    return default_loader_.get();
  return NULL;
}

void ApplicationManager::OnShellImplError(ShellImpl* shell_impl) {
  // Called from ~ShellImpl, so we do not need to call Destroy here.
  const GURL url = shell_impl->url();
  const GURL requested_url = shell_impl->requested_url();
  // Remove the shell.
  URLToShellImplMap::iterator it = url_to_shell_impl_.find(url);
  DCHECK(it != url_to_shell_impl_.end());
  delete it->second;
  url_to_shell_impl_.erase(it);
  ApplicationLoader* loader = GetLoaderForURL(requested_url,
                                              INCLUDE_DEFAULT_LOADER);
  if (loader)
    loader->OnApplicationError(this, url);
  delegate_->OnApplicationError(requested_url);
}

void ApplicationManager::OnContentHandlerError(
    ContentHandlerConnection* content_handler) {
  // Remove the mapping to the content handler.
  auto it =
      url_to_content_handler_.find(content_handler->content_handler_url());
  DCHECK(it != url_to_content_handler_.end());
  delete it->second;
  url_to_content_handler_.erase(it);
}

ScopedMessagePipeHandle ApplicationManager::ConnectToServiceByName(
    const GURL& application_url,
    const std::string& interface_name) {
  ServiceProviderPtr services;
  ConnectToApplication(application_url, GURL(), GetProxy(&services), nullptr);
  MessagePipe pipe;
  services->ConnectToService(interface_name, pipe.handle1.Pass());
  return pipe.handle0.Pass();
}

Array<String> ApplicationManager::GetArgsForURL(const GURL& url) {
  URLToArgsMap::const_iterator args_it = url_to_args_.find(url);
  if (args_it != url_to_args_.end())
    return Array<String>::From(args_it->second);
  return Array<String>();
}
}  // namespace mojo
