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

namespace {

template <typename T>
void Ping(T* service) {
  base::RunLoop run_loop;
  (*service)
      ->Ping(base::Bind(&base::RunLoop::Quit, base::Unretained(&run_loop)));
  run_loop.Run();
}

class ReaperAppTest : public mojo::test::ApplicationTestBase {
 public:
  ReaperAppTest() : ApplicationTestBase() {}
  ~ReaperAppTest() override {}

  void SetUp() override {
    mojo::test::ApplicationTestBase::SetUp();
    application_impl()->ConnectToService("mojo:reaper", &reaper_);
    application_impl()->ConnectToService("mojo:reaper", &diagnostics_);
    diagnostics_->Reset();
    Ping(&diagnostics_);
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

struct SecretCatcher {
  SecretCatcher(uint64* secret) : secret(secret) {}
  void Run(uint64 secret) const { *(this->secret) = secret; }
  uint64* secret;
};

NodePtr CreateNode(const std::string& app_url,
                   uint32 node_id,
                   const std::string& other_app_url,
                   uint32 other_node_id,
                   bool is_source) {
  NodePtr result(Node::New());
  result->app_url = app_url;
  result->node_id = node_id;
  result->other_app_url = other_app_url;
  result->other_id = other_node_id;
  result->is_source = is_source;
  return result.Pass();
}

bool CompareNodes(const NodePtr* a, const NodePtr* b) {
  if ((*a)->app_url < (*b)->app_url)
    return true;
  else if ((*a)->app_url == (*b)->app_url && (*a)->node_id < (*b)->node_id)
    return true;
  else
    return false;
}

std::vector<const NodePtr*> GetNodeVec(const mojo::Array<NodePtr>& node_arr) {
  std::vector<const NodePtr*> result;
  for (size_t i = 0; i < node_arr.size(); ++i)
    result.push_back(&node_arr[i]);
  std::sort(result.begin(), result.end(), CompareNodes);
  return result;
}

void AddReference(mojo::Array<NodePtr>* nodes,
                  const std::string& source_app,
                  uint32 source_node_id,
                  const std::string& dest_app,
                  uint32 dest_node_id) {
  nodes->push_back(CreateNode(source_app, source_node_id, dest_app,
                              dest_node_id, true).Pass());
  nodes->push_back(CreateNode(dest_app, dest_node_id, source_app,
                              source_node_id, false).Pass());
}

void DumpNodes(DiagnosticsPtr* diagnostics, mojo::Array<NodePtr>* nodes) {
  nodes->reset();
  (*diagnostics)->DumpNodes(NodeCatcher(nodes));
  Ping(diagnostics);
}

void ExpectEqual(const std::vector<const NodePtr*>& expected,
                 const std::vector<const NodePtr*>& actual,
                 const std::string& message) {
  EXPECT_EQ(expected.size(), actual.size()) << message;
  for (size_t i = 0; i < expected.size(); ++i) {
    const NodePtr& ex = *expected[i];
    const NodePtr& ac = *actual[i];
    EXPECT_TRUE(ex.Equals(ac)) << i << ": " << message;
  }
}

}  // namespace

TEST_F(ReaperAppTest, CreateAndRead) {
  reaper_->CreateReference(1u, 2u);
  Ping(&reaper_);

  mojo::Array<NodePtr> nodes;
  DumpNodes(&diagnostics_, &nodes);

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

  EXPECT_EQ(1,
            std::count_if(lookup.begin(), lookup.end(), [&is_self](Node* node) {
    return is_self(node->app_url) && node->node_id == 1u &&
           is_self(node->other_app_url) && node->other_id == 2u &&
           node->is_source;
  }));
  EXPECT_EQ(1,
            std::count_if(lookup.begin(), lookup.end(), [&is_self](Node* node) {
    return is_self(node->app_url) && node->node_id == 2u &&
           is_self(node->other_app_url) && node->other_id == 1u &&
           !node->is_source;
  }));
}

TEST_F(ReaperAppTest, CreateDuplicate) {
  reaper_->CreateReference(1u, 2u);

  // The duplicate nodes should be silently ignored.
  reaper_->CreateReference(1u, 2u);
  Ping(&reaper_);

  mojo::Array<NodePtr> nodes;
  DumpNodes(&diagnostics_, &nodes);

  ASSERT_EQ(2u, nodes.size());
}

TEST_F(ReaperAppTest, DropOneNode) {
  reaper_->CreateReference(1u, 2u);
  reaper_->DropNode(1u);
  Ping(&reaper_);

  mojo::Array<NodePtr> nodes;
  DumpNodes(&diagnostics_, &nodes);

  // The other node gets dropped immediately.
  ASSERT_EQ(0u, nodes.size());
}

TEST_F(ReaperAppTest, GetApplicationSecret) {
  uint64 secret1 = 0u;
  reaper_->GetApplicationSecret(SecretCatcher(&secret1));
  Ping(&reaper_);
  EXPECT_NE(0u, secret1);

  uint64 secret2 = 0u;
  reaper_->GetApplicationSecret(SecretCatcher(&secret2));
  Ping(&reaper_);
  EXPECT_EQ(secret1, secret2);

  diagnostics_->Reset();
  Ping(&diagnostics_);
  uint64 secret3 = 0u;
  reaper_->GetApplicationSecret(SecretCatcher(&secret3));
  Ping(&reaper_);
  EXPECT_NE(0u, secret3);
  EXPECT_NE(secret1, secret3);
  EXPECT_NE(secret2, secret3);
}

TEST_F(ReaperAppTest, Transfer) {
  std::string app1_url("http://a.com/1");
  std::string app2_url("http://a.com/2");
  std::string reaper_url("mojo:reaper");
  ReaperPtr reaper1;
  ReaperPtr reaper2;
  diagnostics_->GetReaperForApp(app1_url, GetProxy(&reaper1));
  diagnostics_->GetReaperForApp(app2_url, GetProxy(&reaper2));
  reaper1->CreateReference(1u, 2u);
  Ping(&reaper1);

  mojo::Array<NodePtr> expected;
  AddReference(&expected, app1_url, 1u, app1_url, 2u);
  mojo::Array<NodePtr> actual;
  DumpNodes(&diagnostics_, &actual);
  ExpectEqual(GetNodeVec(expected), GetNodeVec(actual), "before start");

  uint64 app2_secret = 0u;
  reaper2->GetApplicationSecret(SecretCatcher(&app2_secret));
  Ping(&reaper2);
  ASSERT_NE(0u, app2_secret);

  TransferPtr transfer;
  reaper1->StartTransfer(1u, GetProxy(&transfer));
  Ping(&reaper1);
  expected.reset();
  AddReference(&expected, reaper_url, 1u, app1_url, 2u);
  DumpNodes(&diagnostics_, &actual);
  ExpectEqual(GetNodeVec(expected), GetNodeVec(actual), "after start");

  transfer->Complete(app2_secret, 1u);
  Ping(&transfer);
  expected.reset();
  AddReference(&expected, app2_url, 1u, app1_url, 2u);
  DumpNodes(&diagnostics_, &actual);
  ExpectEqual(GetNodeVec(expected), GetNodeVec(actual), "after complete");

  // Shouldn't crash or change anything on second call.
  uint64 app1_secret = 0u;
  reaper1->GetApplicationSecret(SecretCatcher(&app1_secret));
  Ping(&reaper1);
  ASSERT_NE(0u, app1_secret);
  transfer->Complete(app1_secret, 3u);
  Ping(&transfer);
  DumpNodes(&diagnostics_, &actual);
  ExpectEqual(GetNodeVec(expected), GetNodeVec(actual), "after reuse transfer");
}

}  // namespace reaper
