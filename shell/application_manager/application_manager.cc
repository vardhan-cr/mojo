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
#include "base/strings/string_util.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/error_handler.h"
#include "mojo/public/interfaces/application/shell.mojom.h"
#include "mojo/services/content_handler/public/interfaces/content_handler.mojom.h"
#include "shell/application_manager/fetcher.h"
#include "shell/application_manager/local_fetcher.h"
#include "shell/application_manager/network_fetcher.h"

namespace mojo {

namespace {
// Used by TestAPI.
bool has_created_instance = false;

GURL StripQueryFromURL(const GURL& url) {
  GURL::Replacements repl;
  repl.SetQueryStr("");
  std::string result = url.ReplaceComponents(repl).spec();

  // Remove the dangling '?' because it's ugly.
  base::ReplaceChars(result, "?", "", &result);
  return GURL(result);
}

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
    MessagePipe pipe;
    content_handler_.Bind(pipe.handle0.Pass());
    services->ConnectToService(ContentHandler::Name_, pipe.handle1.Pass());
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
    : delegate_(delegate), weak_ptr_factory_(this) {
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

  // We check both the mapped and resolved urls for existing shell_impls because
  // external applications can be registered for the unresolved mojo:foo urls.

  GURL mapped_url = delegate_->ResolveMappings(requested_url);
  if (ConnectToRunningApplication(mapped_url, requestor_url, &services,
                                  &exposed_services)) {
    return;
  }

  GURL resolved_url = delegate_->ResolveURL(mapped_url);
  if (ConnectToRunningApplication(resolved_url, requestor_url, &services,
                                  &exposed_services)) {
    return;
  }

  if (ConnectToApplicationWithLoader(requested_url, mapped_url, requestor_url,
                                     &services, &exposed_services,
                                     GetLoaderForURL(mapped_url))) {
    return;
  }

  if (ConnectToApplicationWithLoader(requested_url, resolved_url, requestor_url,
                                     &services, &exposed_services,
                                     GetLoaderForURL(resolved_url))) {
    return;
  }

  if (ConnectToApplicationWithLoader(requested_url, resolved_url, requestor_url,
                                     &services, &exposed_services,
                                     default_loader_.get())) {
    return;
  }

  auto callback = base::Bind(&ApplicationManager::HandleFetchCallback,
                             weak_ptr_factory_.GetWeakPtr(), requested_url,
                             requestor_url, base::Passed(services.Pass()),
                             base::Passed(exposed_services.Pass()));

  if (resolved_url.SchemeIsFile()) {
    new LocalFetcher(resolved_url, StripQueryFromURL(resolved_url),
                     base::Bind(callback, NativeRunner::DontDeleteAppPath));
    return;
  }

  if (!network_service_)
    ConnectToService(GURL("mojo:network_service"), &network_service_);

  new NetworkFetcher(disable_cache_, resolved_url, network_service_.get(),
                     base::Bind(callback, NativeRunner::DeleteAppPath));
}

bool ApplicationManager::ConnectToRunningApplication(
    const GURL& resolved_url,
    const GURL& requestor_url,
    InterfaceRequest<ServiceProvider>* services,
    ServiceProviderPtr* exposed_services) {
  GURL application_url = StripQueryFromURL(resolved_url);
  ShellImpl* shell_impl = GetShellImpl(application_url);
  if (!shell_impl)
    return false;

  ConnectToClient(shell_impl, resolved_url, requestor_url, services->Pass(),
                  exposed_services->Pass());
  return true;
}

bool ApplicationManager::ConnectToApplicationWithLoader(
    const GURL& requested_url,
    const GURL& resolved_url,
    const GURL& requestor_url,
    InterfaceRequest<ServiceProvider>* services,
    ServiceProviderPtr* exposed_services,
    ApplicationLoader* loader) {
  if (!loader)
    return false;

  loader->Load(resolved_url,
               RegisterShell(requested_url, resolved_url, requestor_url,
                             services->Pass(), exposed_services->Pass()));
  return true;
}

InterfaceRequest<Application> ApplicationManager::RegisterShell(
    const GURL& original_url,
    const GURL& resolved_url,
    const GURL& requestor_url,
    InterfaceRequest<ServiceProvider> services,
    ServiceProviderPtr exposed_services) {
  GURL app_url = StripQueryFromURL(resolved_url);

  ApplicationPtr application;
  InterfaceRequest<Application> application_request = GetProxy(&application);
  ShellImpl* shell =
      new ShellImpl(application.Pass(), this, original_url, app_url);
  url_to_shell_impl_[app_url] = shell;
  shell->InitializeApplication(GetArgsForURL(original_url));
  ConnectToClient(shell, resolved_url, requestor_url, services.Pass(),
                  exposed_services.Pass());
  return application_request.Pass();
}

ShellImpl* ApplicationManager::GetShellImpl(const GURL& url) {
  const auto& shell_it = url_to_shell_impl_.find(url);
  if (shell_it != url_to_shell_impl_.end())
    return shell_it->second;
  return nullptr;
}

void ApplicationManager::ConnectToClient(
    ShellImpl* shell_impl,
    const GURL& resolved_url,
    const GURL& requestor_url,
    InterfaceRequest<ServiceProvider> services,
    ServiceProviderPtr exposed_services) {
  shell_impl->ConnectToClient(resolved_url, requestor_url, services.Pass(),
                              exposed_services.Pass());
}

void ApplicationManager::HandleFetchCallback(
    const GURL& requested_url,
    const GURL& requestor_url,
    InterfaceRequest<ServiceProvider> services,
    ServiceProviderPtr exposed_services,
    NativeRunner::CleanupBehavior cleanup_behavior,
    scoped_ptr<Fetcher> fetcher) {
  if (!fetcher) {
    // Network error. Drop |application_request| to tell requestor.
    return;
  }

  GURL redirect_url = fetcher->GetRedirectURL();
  if (!redirect_url.is_empty()) {
    // And around we go again... Whee!
    ConnectToApplication(redirect_url, requestor_url, services.Pass(),
                         exposed_services.Pass());
    return;
  }

  // We already checked if the application was running before we fetched it, but
  // it might have started while the fetch was outstanding. We don't want to
  // have two copies of the app running, so check again.
  //
  // Also, it's possible the original URL was redirected to an app that is
  // already running.
  if (ConnectToRunningApplication(fetcher->GetURL(), requestor_url, &services,
                                  &exposed_services)) {
    return;
  }

  InterfaceRequest<Application> request(
      RegisterShell(requested_url, fetcher->GetURL(), requestor_url,
                    services.Pass(), exposed_services.Pass()));

  // If the response begins with a #!mojo <content-handler-url>, use it.
  GURL content_handler_url;
  std::string shebang;
  if (fetcher->PeekContentHandler(&shebang, &content_handler_url)) {
    LoadWithContentHandler(
        content_handler_url, request.Pass(),
        fetcher->AsURLResponse(blocking_pool_,
                               static_cast<int>(shebang.size())));
    return;
  }

  MimeTypeToURLMap::iterator iter = mime_type_to_url_.find(fetcher->MimeType());
  if (iter != mime_type_to_url_.end()) {
    LoadWithContentHandler(iter->second, request.Pass(),
                           fetcher->AsURLResponse(blocking_pool_, 0));
    return;
  }

  // TODO(aa): Sanity check that the thing we got looks vaguely like a mojo
  // application. That could either mean looking for the platform-specific dll
  // header, or looking for some specific mojo signature prepended to the
  // library.

  fetcher->AsPath(
      blocking_pool_,
      base::Bind(&ApplicationManager::RunNativeApplication,
                 weak_ptr_factory_.GetWeakPtr(), base::Passed(request.Pass()),
                 cleanup_behavior, base::Passed(fetcher.Pass())));
}

void ApplicationManager::RunNativeApplication(
    InterfaceRequest<Application> application_request,
    NativeRunner::CleanupBehavior cleanup_behavior,
    scoped_ptr<Fetcher> fetcher,
    const base::FilePath& path,
    bool path_exists) {
  // We only passed fetcher to keep it alive. Done with it now.
  fetcher.reset();

  DCHECK(application_request.is_pending());

  if (!path_exists) {
    LOG(ERROR) << "Library not started because library path '" << path.value()
               << "' does not exist.";
    return;
  }

  NativeRunner* runner = native_runner_factory_->Create().release();
  native_runners_.push_back(runner);
  runner->Start(path, cleanup_behavior, application_request.Pass(),
                base::Bind(&ApplicationManager::CleanupRunner,
                           weak_ptr_factory_.GetWeakPtr(), runner));
}

void ApplicationManager::RegisterExternalApplication(
    const GURL& url,
    const std::vector<std::string>& args,
    ApplicationPtr application) {
  const auto& args_it = url_to_args_.find(url);
  if (args_it != url_to_args_.end()) {
    LOG(WARNING) << "--args-for provided for external application " << url
                 << " <ignored>";
  }
  ShellImpl* shell_impl = new ShellImpl(application.Pass(), this, url, url);
  url_to_shell_impl_[url] = shell_impl;
  shell_impl->InitializeApplication(Array<String>::From(args));
}

void ApplicationManager::RegisterContentHandler(
    const std::string& mime_type,
    const GURL& content_handler_url) {
  DCHECK(content_handler_url.is_valid())
      << "Content handler URL is invalid for mime type " << mime_type;
  mime_type_to_url_[mime_type] = content_handler_url;
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

ApplicationLoader* ApplicationManager::GetLoaderForURL(const GURL& url) {
  auto url_it = url_to_loader_.find(StripQueryFromURL(url));
  if (url_it != url_to_loader_.end())
    return url_it->second;
  auto scheme_it = scheme_to_loader_.find(url.scheme());
  if (scheme_it != scheme_to_loader_.end())
    return scheme_it->second;
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
  const auto& args_it = url_to_args_.find(url);
  if (args_it != url_to_args_.end())
    return Array<String>::From(args_it->second);
  return Array<String>();
}

void ApplicationManager::CleanupRunner(NativeRunner* runner) {
  native_runners_.erase(
      std::find(native_runners_.begin(), native_runners_.end(), runner));
}

}  // namespace mojo
