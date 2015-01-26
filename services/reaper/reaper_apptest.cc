// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "mojo/common/common_type_converters.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "services/reaper/diagnostics.mojom.h"
#include "services/reaper/reaper.mojom.h"

namespace reaper {

class ReaperAppTest : public mojo::test::ApplicationTestBase {
 public:
  ReaperAppTest() : ApplicationTestBase() {}
  ~ReaperAppTest() override {}

  void SetUp() override {
    mojo::test::ApplicationTestBase::SetUp();
    application_impl()->ConnectToService("mojo:reaper", &reaper_);
    application_impl()->ConnectToService("mojo:reaper", &diagnostics_);

    diagnostics_->Reset(mojo::Callback<void()>());
    diagnostics_.WaitForIncomingMethodCall();
  }

 protected:
  ReaperPtr reaper_;
  DiagnosticsPtr diagnostics_;

  DISALLOW_COPY_AND_ASSIGN(ReaperAppTest);
};

struct NodeCatcher {
  NodeCatcher(mojo::Array<NodePtr>* nodes) : nodes(nodes) {}
  void Run(mojo::Array<NodePtr> nodes) const { *(this->nodes) = nodes.Pass(); }
  mojo::Array<NodePtr>* nodes;
};

TEST_F(ReaperAppTest, CreateAndRead) {
  reaper_->CreateReference(1u, 2u);

  mojo::Array<NodePtr> nodes;
  diagnostics_->DumpNodes(NodeCatcher(&nodes));
  diagnostics_.WaitForIncomingMethodCall();

  ASSERT_EQ(2u, nodes.size());
  // TODO(aa): Need to build STL iteration into mojo::Array.
  std::vector<Node*> lookup;
  for (size_t i = 0; i < nodes.size(); ++i)
    lookup.push_back(nodes[i].get());

  // TODO(aa): It would be good to test complete URL, but we don't have it yet
  // because native apps aren't implemented via the ContentHandler interface.
  auto is_self = [](const std::string& url) {
    return EndsWith(url, "/reaper_apptests.mojo", false);
  };

  EXPECT_EQ(1u,
            std::count_if(lookup.begin(), lookup.end(), [&is_self](Node* node) {
    return is_self(node->app_url) && node->node_id == 1u &&
           is_self(node->other_app_url) && node->other_id == 2u &&
           node->is_source && !node->is_being_transferred;
  }));
  EXPECT_EQ(1u,
            std::count_if(lookup.begin(), lookup.end(), [&is_self](Node* node) {
    return is_self(node->app_url) && node->node_id == 2u &&
           is_self(node->other_app_url) && node->other_id == 1u &&
           !node->is_source && !node->is_being_transferred;
  }));
}

TEST_F(ReaperAppTest, CreateDuplicate) {
  reaper_->CreateReference(1u, 2u);

  // The duplicate nodes should be silently ignored.
  reaper_->CreateReference(1u, 2u);

  mojo::Array<NodePtr> nodes;
  diagnostics_->DumpNodes(NodeCatcher(&nodes));
  diagnostics_.WaitForIncomingMethodCall();

  ASSERT_EQ(2u, nodes.size());
}

TEST_F(ReaperAppTest, DropOneNode) {
  reaper_->CreateReference(1u, 2u);

  // For now, because we don't have GC, this should just result in us having a
  // dangling node. Later, it will result in other node, and perhaps other refs
  // also being dropped.
  reaper_->DropNode(1u);

  mojo::Array<NodePtr> nodes;
  diagnostics_->DumpNodes(NodeCatcher(&nodes));
  diagnostics_.WaitForIncomingMethodCall();

  ASSERT_EQ(1u, nodes.size());
  std::vector<Node*> lookup;
  for (size_t i = 0; i < nodes.size(); ++i)
    lookup.push_back(nodes[i].get());
  EXPECT_EQ(1u, std::count_if(lookup.begin(), lookup.end(),
                              [](Node* node) { return node->node_id == 2u; }));
}

}  // namespace reaper
