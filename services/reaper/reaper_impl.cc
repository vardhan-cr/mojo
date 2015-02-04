// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/reaper/reaper_impl.h"

#include "base/logging.h"
#include "base/stl_util.h"
#include "crypto/random.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "services/reaper/reaper_binding.h"
#include "services/reaper/transfer_binding.h"

namespace reaper {

struct ReaperImpl::InternedURL {
  bool operator<(const InternedURL& other) const { return url < other.url; }
  GURL* url = NULL;
};

struct ReaperImpl::NodeLocator {
  NodeLocator() : node_id(0) {}
  NodeLocator(const NodeLocator& other) = default;
  NodeLocator(InternedURL app_url, uint32 node_id)
      : app_url(app_url), node_id(node_id) {}
  InternedURL app_url;
  uint32 node_id;
};

struct ReaperImpl::NodeInfo {
  NodeInfo() : is_source(false) {}
  NodeInfo(const NodeInfo& other) = default;
  NodeInfo(const NodeLocator& other_node)
      : other_node(other_node), is_source(false) {}
  NodeLocator other_node;
  bool is_source;
};

ReaperImpl::ReaperImpl() : reaper_url_("mojo:reaper"), next_transfer_id_(1) {
}

ReaperImpl::~ReaperImpl() {
  STLDeleteValues(&interned_urls_);
}

ReaperImpl::InternedURL ReaperImpl::InternURL(const GURL& url) {
  auto interned = interned_urls_[url];
  if (!interned) {
    interned = new GURL(url);
    interned_urls_[url] = interned;
  }
  InternedURL result;
  result.url = interned;
  return result;
}

ReaperImpl::NodeInfo* ReaperImpl::GetNode(
    const ReaperImpl::NodeLocator& locator) {
  auto app = nodes_.find(locator.app_url);
  if (app == nodes_.end())
    return NULL;

  auto node = app->second.find(locator.node_id);
  if (node == app->second.end())
    return NULL;

  return &(node->second);
}

bool ReaperImpl::MoveNode(const ReaperImpl::NodeLocator& source_locator,
                          const ReaperImpl::NodeLocator& dest_locator) {
  NodeInfo* source = GetNode(source_locator);
  if (!source) {
    return false;
  }

  NodeInfo* dest = GetNode(dest_locator);
  if (dest) {
    return false;
  }

  NodeInfo* other = GetNode(source->other_node);
  DCHECK(other);

  nodes_[dest_locator.app_url][dest_locator.node_id] = *source;
  nodes_[source_locator.app_url].erase(source_locator.node_id);
  other->other_node = dest_locator;
  return true;
}

void ReaperImpl::GetApplicationSecret(
    const GURL& caller_app,
    const mojo::Callback<void(AppSecret)>& callback) {
  InternedURL app_url = InternURL(caller_app);
  AppSecret secret = app_url_to_secret_[app_url];
  if (secret == 0u) {
    crypto::RandBytes(&secret, sizeof(AppSecret));
    CHECK_NE(secret, 0u);
    app_url_to_secret_[app_url] = secret;
    app_secret_to_url_[secret] = app_url;
  }
  callback.Run(secret);
}

void ReaperImpl::CreateReference(const GURL& caller_app,
                                 uint32 source_node_id,
                                 uint32 target_node_id) {
  InternedURL app_url = InternURL(caller_app);

  NodeLocator source_locator(app_url, source_node_id);
  NodeLocator target_locator(app_url, target_node_id);

  if (GetNode(source_locator) != NULL) {
    LOG(ERROR) << "Duplicate source node: " << source_node_id;
    return;
  }

  if (GetNode(target_locator) != NULL) {
    LOG(ERROR) << "Duplicate target node: " << target_node_id;
    return;
  }

  NodeInfo source_node(target_locator);
  source_node.is_source = true;
  nodes_[source_locator.app_url][source_locator.node_id] = source_node;

  NodeInfo target_node(source_locator);
  nodes_[target_locator.app_url][target_locator.node_id] = target_node;
}

void ReaperImpl::DropNode(const GURL& caller_app, uint32 node_id) {
  NodeLocator locator(InternURL(caller_app), node_id);
  NodeInfo* node = GetNode(locator);
  if (!node) {
    LOG(ERROR) << "Specified node does not exist: " << node_id;
    return;
  }

  NodeLocator other_locator = node->other_node;
  DCHECK(GetNode(other_locator));
  nodes_[locator.app_url].erase(locator.node_id);
  nodes_[other_locator.app_url].erase(other_locator.node_id);
}

void ReaperImpl::StartTransfer(const GURL& caller_app,
                               uint32 node_id,
                               mojo::InterfaceRequest<Transfer> request) {
  uint32 transfer_id = next_transfer_id_++;
  NodeLocator source(InternURL(caller_app), node_id);
  if (!MoveNode(source, NodeLocator(InternURL(reaper_url_), transfer_id))) {
    LOG(ERROR) << "Could not start node transfer because move failed from: ("
               << caller_app.spec() << "," << node_id << ") to: ("
               << reaper_url_.spec() << "," << transfer_id << ")";
    return;
  }
  new TransferBinding(transfer_id, this, request.Pass());
}

void ReaperImpl::CompleteTransfer(uint32 source_node_id,
                                  uint64 dest_app_secret,
                                  uint32 dest_node_id) {
  InternedURL dest_app_url = app_secret_to_url_[dest_app_secret];
  if (!dest_app_url.url) {
    LOG(ERROR) << "Specified destination app secret does not exist: "
               << dest_app_secret;
    return;
  }

  InternedURL source_app_url(InternURL(reaper_url_));
  NodeLocator source(source_app_url, source_node_id);
  NodeLocator dest(dest_app_url, dest_node_id);
  if (!MoveNode(source, dest)) {
    LOG(ERROR) << "Could not complete transfer because move failed from: ("
               << *source_app_url.url << "," << source_node_id << ") to: ("
               << *dest_app_url.url << "," << dest_node_id << ")";
  }
}

bool ReaperImpl::ConfigureIncomingConnection(
    mojo::ApplicationConnection* connection) {
  connection->AddService<Reaper>(this);
  connection->AddService<Diagnostics>(this);
  return true;
}

void ReaperImpl::Create(mojo::ApplicationConnection* connection,
                        mojo::InterfaceRequest<Reaper> request) {
  GetReaperForApp(connection->GetRemoteApplicationURL(), request.Pass());
}

void ReaperImpl::Create(mojo::ApplicationConnection* connection,
                        mojo::InterfaceRequest<Diagnostics> request) {
  // TODO(aa): Enforce that only testing code can connect to this interface.
  diagnostics_bindings_.AddBinding(this, request.Pass());
}

void ReaperImpl::DumpNodes(
    const mojo::Callback<void(mojo::Array<NodePtr>)>& callback) {
  mojo::Array<NodePtr> result(0u);
  for (const auto& app : nodes_) {
    for (const auto& node_info : app.second) {
      NodePtr node(Node::New());
      node->app_url = app.first.url->spec();
      node->node_id = node_info.first;
      node->other_app_url = node_info.second.other_node.app_url.url->spec();
      node->other_id = node_info.second.other_node.node_id;
      node->is_source = node_info.second.is_source;
      result.push_back(node.Pass());
    }
  }
  callback.Run(result.Pass());
}

void ReaperImpl::Reset() {
  nodes_.clear();
  app_url_to_secret_.clear();
  app_secret_to_url_.clear();
}

void ReaperImpl::GetReaperForApp(const mojo::String& app_url,
                                 mojo::InterfaceRequest<Reaper> request) {
  new ReaperBinding(GURL(app_url), this, request.Pass());
}

void ReaperImpl::Ping(const mojo::Closure& closure) {
  closure.Run();
}

}  // namespace reaper
