// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/run_loop.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/reaper/diagnostics.mojom.h"
#include "services/reaper/reaper.mojom.h"
#include "services/reaper/scythe.mojom.h"
#include "services/reaper/transfer.mojom.h"

namespace reaper {

namespace {

template <typename T>
void Ping(T* service) {
  base::RunLoop run_loop;
  (*service)
      ->Ping(base::Bind(&base::RunLoop::Quit, base::Unretained(&run_loop)));
  run_loop.Run();
}

struct SecretCatcher {
  SecretCatcher(uint64* secret) : secret(secret) {}
  void Run(uint64 secret) const { *(this->secret) = secret; }
  uint64* secret;
};

class ScytheImpl : public Scythe {
 public:
  ScytheImpl(mojo::InterfaceRequest<Scythe> request)
      : binding_(this, request.Pass()) {}
  void WaitForKills(size_t num) {
    while (deds.size() < num) {
      binding_.WaitForIncomingMethodCall();
    }
  }
  std::vector<std::string> deds;

 private:
  void KillApplication(const mojo::String& url) override {
    deds.push_back(url);
    std::sort(deds.begin(), deds.end());
  }
  void Ping(const mojo::Closure& callback) override { callback.Run(); }
  mojo::StrongBinding<Scythe> binding_;
};

class ReaperAppTest : public mojo::test::ApplicationTestBase {
 public:
  ReaperAppTest() : ApplicationTestBase(), scythe_(NULL) {}
  ~ReaperAppTest() override {}

  void SetUp() override {
    mojo::test::ApplicationTestBase::SetUp();
    application_impl()->ConnectToService("mojo:reaper", &diagnostics_);

    diagnostics_->Reset();
    ScythePtr scythe_ptr;
    scythe_ = new ScytheImpl(GetProxy(&scythe_ptr).Pass());
    diagnostics_->SetScythe(scythe_ptr.Pass());
    Ping(&diagnostics_);

    diagnostics_->GetReaperForApp(app1_url_, GetProxy(&reaper1_));
    diagnostics_->GetReaperForApp(app2_url_, GetProxy(&reaper2_));
    diagnostics_->GetReaperForApp(app3_url_, GetProxy(&reaper3_));
    reaper1_->GetApplicationSecret(SecretCatcher(&app1_secret_));
    reaper2_->GetApplicationSecret(SecretCatcher(&app2_secret_));
    reaper3_->GetApplicationSecret(SecretCatcher(&app3_secret_));
    Ping(&reaper1_);
    Ping(&reaper2_);
    Ping(&reaper3_);
  }

 protected:
  TransferPtr StartTransfer(ReaperPtr* reaper, uint32 node_id) {
    TransferPtr transfer;
    (*reaper)->StartTransfer(node_id, GetProxy(&transfer));
    Ping(reaper);
    return transfer.Pass();
  }

  void CompleteTransfer(TransferPtr* transfer,
                        uint64 app_secret,
                        uint32 node_id) {
    (*transfer)->Complete(app_secret, node_id);
    Ping(transfer);
  }

  void Transfer(ReaperPtr* from_app_reaper,
                uint32 from_node,
                uint64 to_app_secret,
                uint32 to_node) {
    TransferPtr transfer(StartTransfer(from_app_reaper, from_node));
    CompleteTransfer(&transfer, to_app_secret, to_node);
  }

  ScytheImpl* scythe_;
  DiagnosticsPtr diagnostics_;
  std::string app1_url_ = "https://a1/";
  std::string app2_url_ = "https://a2/";
  std::string app3_url_ = "https://a3/";
  std::string reaper_url_ = "mojo:reaper";
  uint64 app1_secret_;
  uint64 app2_secret_;
  uint64 app3_secret_;
  ReaperPtr reaper1_;
  ReaperPtr reaper2_;
  ReaperPtr reaper3_;

  DISALLOW_COPY_AND_ASSIGN(ReaperAppTest);
};

struct NodeCatcher {
  NodeCatcher(mojo::Array<NodePtr>* nodes) : nodes(nodes) {}
  void Run(mojo::Array<NodePtr> nodes) const { *(this->nodes) = nodes.Pass(); }
  mojo::Array<NodePtr>* nodes;
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

void ExpectEqual(const mojo::Array<NodePtr>& expected,
                 const mojo::Array<NodePtr>& actual,
                 const std::string& message) {
  auto expected_vec = GetNodeVec(expected);
  auto actual_vec = GetNodeVec(actual);
  EXPECT_EQ(expected_vec.size(), actual_vec.size()) << message;
  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_TRUE(expected_vec[i]->Equals(*actual_vec[i])) << message << " : "
                                                         << i;
  }
}

void ExpectEqual(const mojo::Array<NodePtr>& expected,
                 const mojo::Array<NodePtr>& actual) {
  ExpectEqual(expected, actual, "");
}

}  // namespace

TEST_F(ReaperAppTest, CreateAndRead) {
  reaper1_->CreateReference(1u, 2u);
  Ping(&reaper1_);

  mojo::Array<NodePtr> actual;
  DumpNodes(&diagnostics_, &actual);
  ASSERT_EQ(2u, actual.size());

  mojo::Array<NodePtr> expected;
  expected.push_back(CreateNode(app1_url_, 1u, app1_url_, 2u, true).Pass());
  expected.push_back(CreateNode(app1_url_, 2u, app1_url_, 1u, false).Pass());
  ExpectEqual(expected, actual);
}

TEST_F(ReaperAppTest, CreateDuplicate) {
  reaper1_->CreateReference(1u, 2u);

  // The duplicate nodes should be silently ignored.
  reaper1_->CreateReference(1u, 2u);
  Ping(&reaper1_);

  mojo::Array<NodePtr> nodes;
  DumpNodes(&diagnostics_, &nodes);
  ASSERT_EQ(2u, nodes.size());
}

TEST_F(ReaperAppTest, DropOneNode) {
  reaper1_->CreateReference(1u, 2u);
  reaper1_->DropNode(1u);
  Ping(&reaper1_);

  mojo::Array<NodePtr> nodes;
  DumpNodes(&diagnostics_, &nodes);

  // The other node gets dropped immediately.
  ASSERT_EQ(0u, nodes.size());
}

TEST_F(ReaperAppTest, GetApplicationSecret) {
  EXPECT_NE(0u, app1_secret_);
  EXPECT_NE(0u, app2_secret_);

  uint64 secret1b = 0u;
  reaper1_->GetApplicationSecret(SecretCatcher(&secret1b));
  Ping(&reaper1_);
  EXPECT_EQ(app1_secret_, secret1b);
}

TEST_F(ReaperAppTest, Transfer) {
  reaper1_->CreateReference(1u, 2u);
  Ping(&reaper1_);

  diagnostics_->SetIsRoot(app1_url_, true);
  Ping(&diagnostics_);

  mojo::Array<NodePtr> expected;
  AddReference(&expected, app1_url_, 1u, app1_url_, 2u);
  mojo::Array<NodePtr> actual;
  DumpNodes(&diagnostics_, &actual);
  ExpectEqual(expected, actual, "before start");

  TransferPtr transfer(StartTransfer(&reaper1_, 2u));
  expected.reset();
  AddReference(&expected, app1_url_, 1u, reaper_url_, 1u);
  DumpNodes(&diagnostics_, &actual);
  ExpectEqual(expected, actual, "after start");

  CompleteTransfer(&transfer, app2_secret_, 1u);
  expected.reset();
  AddReference(&expected, app1_url_, 1u, app2_url_, 1u);
  DumpNodes(&diagnostics_, &actual);
  ExpectEqual(expected, actual, "after complete");

  // Shouldn't crash or change anything on second call.
  CompleteTransfer(&transfer, app1_secret_, 3u);
  DumpNodes(&diagnostics_, &actual);
  ExpectEqual(expected, actual, "after reuse transfer");
}

TEST_F(ReaperAppTest, Collect1) {
  // Create a reference that goes nowhere and drop it. This triggers a GC, so we
  // should see the app be killed.
  reaper1_->CreateReference(1u, 2u);
  reaper1_->DropNode(1u);
  Ping(&reaper1_);
  scythe_->WaitForKills(1u);
  EXPECT_EQ(1u, scythe_->deds.size());
  EXPECT_EQ(app1_url_, scythe_->deds[0]);

  // Create a ref that goes from a root to itself, and drop it. Nothing should
  // be killed.
  scythe_->deds.clear();
  diagnostics_->SetIsRoot(app1_url_, true);
  reaper1_->CreateReference(3u, 4u);
  reaper1_->CreateReference(5u, 6u);
  reaper1_->DropNode(3u);
  Ping(&reaper1_);

  mojo::Array<NodePtr> expected;
  AddReference(&expected, app1_url_, 5u, app1_url_, 6u);
  mojo::Array<NodePtr> actual;
  DumpNodes(&diagnostics_, &actual);
  ExpectEqual(expected, actual);
}

TEST_F(ReaperAppTest, CollectNodesAreDirected) {
  diagnostics_->SetIsRoot(app1_url_, true);
  Ping(&diagnostics_);
  reaper1_->CreateReference(1u, 2u);

  // Transfer the *source* node to app2. Now app2 is referencing app1.
  Transfer(&reaper1_, 1u, app2_secret_, 1u);

  // Since app2 is not a root, it should get collected.
  scythe_->WaitForKills(1u);
  ASSERT_EQ(1u, scythe_->deds.size());
  EXPECT_EQ(app2_url_, scythe_->deds[0]);

  // Should have cleaned up the graph too.
  mojo::Array<NodePtr> expected;
  mojo::Array<NodePtr> actual;
  DumpNodes(&diagnostics_, &actual);
  ExpectEqual(expected, actual);

}

}  // namespace reaper
