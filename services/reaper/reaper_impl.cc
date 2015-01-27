// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/reaper/reaper_impl.h"

#include "base/logging.h"
#include "crypto/random.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "services/reaper/reaper_binding.h"
#include "services/reaper/transfer_binding.h"

namespace reaper {

struct ReaperImpl::NodeLocator {
  NodeLocator() : node_id(0) {}
  NodeLocator(const NodeLocator& other) = default;
  NodeLocator(AppId app_id, uint32 node_id)
      : app_id(app_id), node_id(node_id) {}
  bool operator<(const NodeLocator& rhs) const {
    if (app_id < rhs.app_id)
      return true;
    else if (app_id == rhs.app_id && node_id < rhs.node_id)
      return true;
    else
      return false;
  }
  AppId app_id;
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

ReaperImpl::ReaperImpl()
    : reaper_url_("mojo:reaper"), next_app_id_(1), next_transfer_id_(1) {
}

ReaperImpl::~ReaperImpl() {
}

ReaperImpl::AppId ReaperImpl::GetAppId(const GURL& app_url) {
  auto result = app_ids_[app_url];
  if (result == 0) {
    result = app_ids_[app_url] = next_app_id_++;
  }
  return result;
}

const GURL& ReaperImpl::GetAppURLSlowly(AppId app_id) const {
  for (const auto& it : app_ids_) {
    if (it.second == app_id)
      return it.first;
  }
  return GURL::EmptyGURL();
}

bool ReaperImpl::MoveNode(const NodeLocator& source, const NodeLocator& dest) {
  const auto& source_it = nodes_.find(source);
  if (source_it == nodes_.end())
    return false;

  const auto& dest_it = nodes_.find(dest);
  if (dest_it != nodes_.end())
    return false;

  nodes_[dest] = source_it->second;
  nodes_.erase(source_it);

  nodes_[nodes_[dest].other_node].other_node = dest;
  return true;
}

void ReaperImpl::GetApplicationSecret(
    const GURL& caller_app,
    const mojo::Callback<void(AppSecret)>& callback) {
  AppId app_id = GetAppId(caller_app);
  AppSecret secret = app_id_to_secret_[app_id];
  if (secret == 0u) {
    crypto::RandBytes(&secret, sizeof(AppSecret));
    CHECK_NE(secret, 0u);
    app_id_to_secret_[app_id] = secret;
    app_secret_to_id_[secret] = app_id;
  }
  callback.Run(secret);
}

void ReaperImpl::CreateReference(const GURL& caller_app,
                                 uint32 source_node_id,
                                 uint32 target_node_id) {
  NodeLocator source_locator(GetAppId(caller_app), source_node_id);
  NodeLocator target_locator(GetAppId(caller_app), target_node_id);

  if (nodes_.find(source_locator) != nodes_.end()) {
    LOG(ERROR) << "Duplicate source node: " << source_node_id;
    return;
  }

  if (nodes_.find(target_locator) != nodes_.end()) {
    LOG(ERROR) << "Duplicate target node: " << target_node_id;
    return;
  }

  NodeInfo source_node(target_locator);
  source_node.is_source = true;
  nodes_[source_locator] = source_node;

  NodeInfo target_node(source_locator);
  nodes_[target_locator] = target_node;
}

void ReaperImpl::DropNode(const GURL& caller_app, uint32 node_id) {
  const auto& it = nodes_.find(NodeLocator(GetAppId(caller_app), node_id));
  if (it == nodes_.end()) {
    LOG(ERROR) << "Specified node does not exist: " << node_id;
    return;
  }

  const auto& other_it = nodes_.find(
      NodeLocator(it->second.other_node.app_id, it->second.other_node.node_id));
  DCHECK(other_it != nodes_.end());
  nodes_.erase(other_it->first);

  nodes_.erase(it);
}

void ReaperImpl::StartTransfer(const GURL& caller_app,
                               uint32 node_id,
                               mojo::InterfaceRequest<Transfer> request) {
  AppId transfer_id = next_transfer_id_++;
  NodeLocator source(GetAppId(caller_app), node_id);
  if (!MoveNode(source, NodeLocator(GetAppId(reaper_url_), transfer_id))) {
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
  AppId dest_app_id = app_secret_to_id_[dest_app_secret];
  if (dest_app_id == 0u) {
    LOG(ERROR) << "Specified destination app secret does not exist: "
               << dest_app_secret;
    return;
  }

  NodeLocator source(GetAppId(reaper_url_), source_node_id);
  NodeLocator dest(dest_app_id, dest_node_id);
  if (!MoveNode(source, dest)) {
    LOG(ERROR) << "Could not complete transfer because move failed from: ("
               << reaper_url_.spec() << "," << source_node_id << ") to: ("
               << GetAppURLSlowly(dest_app_id).spec() << "," << dest_node_id
               << ")";
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
  std::map<AppId, GURL> app_urls;
  for (const auto& it : app_ids_)
    app_urls[it.second] = it.first;
  mojo::Array<NodePtr> result(0u);
  for (const auto& entry : nodes_) {
    NodePtr node(Node::New());
    node->app_url = app_urls[entry.first.app_id].spec();
    node->node_id = entry.first.node_id;
    node->other_app_url = app_urls[entry.second.other_node.app_id].spec();
    node->other_id = entry.second.other_node.node_id;
    node->is_source = entry.second.is_source;
    result.push_back(node.Pass());
  }
  callback.Run(result.Pass());
}

void ReaperImpl::Reset() {
  nodes_.clear();
  app_id_to_secret_.clear();
  app_secret_to_id_.clear();
}

void ReaperImpl::GetReaperForApp(const mojo::String& app_url,
                                 mojo::InterfaceRequest<Reaper> request) {
  new ReaperBinding(GURL(app_url), this, request.Pass());
}

void ReaperImpl::Ping(const mojo::Closure& closure) {
  closure.Run();
}

}  // namespace reaper
