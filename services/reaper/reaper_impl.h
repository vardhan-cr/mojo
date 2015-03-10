// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_REAPER_REAPER_IMPL_H_
#define SERVICES_REAPER_REAPER_IMPL_H_

#include <map>
#include <set>

#include "base/macros.h"
#include "mojo/common/weak_binding_set.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/public/cpp/bindings/array.h"
#include "mojo/public/cpp/bindings/callback.h"
#include "services/reaper/diagnostics.mojom.h"
#include "services/reaper/reaper.mojom.h"
#include "services/reaper/scythe.mojom.h"
#include "url/gurl.h"

namespace mojo {
class ApplicationConnection;
}  // namespace mojo

namespace reaper {

class ReaperImpl : public Diagnostics,
                   public mojo::ApplicationDelegate,
                   public mojo::InterfaceFactory<Reaper>,
                   public mojo::InterfaceFactory<Diagnostics> {
 public:
  typedef uint64 AppSecret;

  ReaperImpl();
  ~ReaperImpl() override;

  void GetApplicationSecret(const GURL& caller_app,
                            const mojo::Callback<void(AppSecret)>&);
  void CreateReference(const GURL& caller_app,
                       uint32 source_node_id,
                       uint32 target_node_id);
  void DropNode(const GURL& caller_app, uint32 node);
  void StartTransfer(const GURL& caller_app,
                     uint32 node,
                     mojo::InterfaceRequest<Transfer> request);
  void CompleteTransfer(uint32 source_node_id,
                        uint64 dest_app_secret,
                        uint32 dest_node_id);

 private:
  struct InternedURL;
  struct NodeLocator;
  struct NodeInfo;

  InternedURL InternURL(const GURL& app_url);
  NodeInfo* GetNode(const NodeLocator& locator);
  bool MoveNode(const NodeLocator& source, const NodeLocator& dest);
  void Mark(InternedURL app_url, std::set<InternedURL>* marked);
  void Collect();

  // mojo::ApplicationDelegate
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override;

  // mojo::InterfaceFactory<Reaper>
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<Reaper> request) override;

  // mojo::InterfaceFactory<Diagnostics>
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<Diagnostics> request) override;

  // Diagnostics
  void DumpNodes(
      const mojo::Callback<void(mojo::Array<NodePtr>)>& callback) override;
  void Reset() override;
  void GetReaperForApp(const mojo::String& url,
                       mojo::InterfaceRequest<Reaper> request) override;
  void Ping(const mojo::Closure& callback) override;
  void SetIsRoot(const mojo::String& url, bool is_root) override;
  void SetScythe(ScythePtr scythe) override;

  GURL reaper_url_;

  // There will be a lot of nodes in a running system, so we intern app urls.
  std::map<GURL, GURL*> interned_urls_;

  // These are the ids assigned to nodes while they are being transferred.
  uint32 next_transfer_id_;

  std::map<InternedURL, AppSecret> app_url_to_secret_;
  std::map<AppSecret, InternedURL> app_secret_to_url_;

  // TODO(aa): The values in at least the first-level map should probably be
  // heap-allocated.
  typedef std::map<uint32, NodeInfo> NodeMap;
  typedef std::map<InternedURL, NodeMap> AppMap;
  AppMap nodes_;

  mojo::WeakBindingSet<Diagnostics> diagnostics_bindings_;

  std::set<InternedURL> roots_;
  ScythePtr scythe_;

  DISALLOW_COPY_AND_ASSIGN(ReaperImpl);
};

#endif  // SERVICES_REAPER_REAPER_IMPL_H_

}  // namespace reaper
