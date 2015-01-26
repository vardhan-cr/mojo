// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/reaper/reaper_impl.h"

#include "base/logging.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "services/reaper/reaper_binding.h"

namespace reaper {

struct ReaperImpl::NodeLocator {
  NodeLocator() : id(0) {}
  NodeLocator(const NodeLocator& other) = default;
  NodeLocator(const GURL& app_url, uint32 id) : app_url(app_url), id(id) {}
  bool operator<(const NodeLocator& rhs) const {
    if (app_url < rhs.app_url)
      return true;
    else if (app_url == rhs.app_url && id < rhs.id)
      return true;
    else
      return false;
  }
  GURL app_url;
  uint32 id;
};

struct ReaperImpl::NodeInfo {
  NodeInfo() : is_source(false), is_being_transferred(false) {}
  NodeInfo(const NodeInfo& other) = default;
  NodeInfo(const NodeLocator& other_node)
      : other_node(other_node), is_source(false), is_being_transferred(false) {}
  NodeLocator other_node;
  bool is_source;
  bool is_being_transferred;
};

ReaperImpl::ReaperImpl() {
}

ReaperImpl::~ReaperImpl() {
}

void ReaperImpl::CreateReference(GURL caller_app,
                                 uint32 source_node_id,
                                 uint32 target_node_id) {
  NodeLocator source_locator(caller_app, source_node_id);
  NodeLocator target_locator(caller_app, target_node_id);

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

void ReaperImpl::DropNode(GURL caller_app, uint32 node_id) {
  const auto& it = nodes_.find(NodeLocator(caller_app, node_id));
  if (it == nodes_.end()) {
    LOG(ERROR) << "Specified node does not exist: " << node_id;
    return;
  }
  nodes_.erase(it);
}

bool ReaperImpl::ConfigureIncomingConnection(
    mojo::ApplicationConnection* connection) {
  connection->AddService<Reaper>(this);
  connection->AddService<Diagnostics>(this);
  return true;
}

void ReaperImpl::Create(mojo::ApplicationConnection* connection,
                        mojo::InterfaceRequest<Reaper> request) {
  new ReaperBinding(GURL(connection->GetRemoteApplicationURL()), this,
                    request.Pass());
}

void ReaperImpl::Create(mojo::ApplicationConnection* connection,
                        mojo::InterfaceRequest<Diagnostics> request) {
  // TODO(aa): Enforce that only testing code can connect to this interface.
  diagnostics_bindings_.AddBinding(this, request.Pass());
}

void ReaperImpl::DumpNodes(
    const mojo::Callback<void(mojo::Array<NodePtr>)>& callback) {
  mojo::Array<NodePtr> result(0u);
  for (const auto& entry : nodes_) {
    NodePtr node(Node::New());
    node->app_url = entry.first.app_url.spec();
    node->id = entry.first.id;
    node->other_app_url = entry.second.other_node.app_url.spec();
    node->other_id = entry.second.other_node.id;
    node->is_source = entry.second.is_source;
    node->is_being_transferred = entry.second.is_being_transferred;
    result.push_back(node.Pass());
  }
  callback.Run(result.Pass());
}

void ReaperImpl::Reset(const mojo::Callback<void()>& callback) {
  nodes_.clear();
  callback.Run();
}

}  // namespace reaper
