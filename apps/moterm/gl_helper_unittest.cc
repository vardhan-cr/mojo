// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Note: This test depends on |GlHelper|, which depends on an implementation of
// mojo:native_viewport_service and mojo:surfaces_service.

#include "apps/moterm/gl_helper.h"

#include <GLES2/gl2.h>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/services/surfaces/public/interfaces/surface_id.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class GlHelperTest : public mojo::test::ApplicationTestBase,
                     public GlHelper::Client {
 public:
  GlHelperTest()
      : surface_id_changed_call_count_(0),
        on_frame_displayed_call_count_(0),
        last_frame_id_(0) {}
  ~GlHelperTest() override {}

  void SetUp() override {
    mojo::test::ApplicationTestBase::SetUp();

    initial_size_.width = 100;
    initial_size_.height = 100;
    gl_helper_.reset(new GlHelper(this, application_impl()->shell(), GL_RGBA,
                                  initial_size_));
  }

 protected:
  const mojo::Size& initial_size() const { return initial_size_; }
  GlHelper* gl_helper() { return gl_helper_.get(); }

  unsigned surface_id_changed_call_count() const {
    return surface_id_changed_call_count_;
  }
  const mojo::SurfaceId* last_surface_id() const {
    return last_surface_id_.get();
  }
  unsigned on_frame_displayed_call_count() const {
    return on_frame_displayed_call_count_;
  }
  uint32_t last_frame_id() const { return last_frame_id_; }

 private:
  // |GlHelper::Client|:
  void OnSurfaceIdChanged(mojo::SurfaceIdPtr surface_id) override {
    surface_id_changed_call_count_++;
    last_surface_id_ = surface_id.Pass();
    base::MessageLoop::current()->Quit();
  }

  void OnContextLost() override {
    // We don't currently test this, but we'll get it on teardown.
    // TODO(vtl): We probably should figure out how to test this explicitly.
  }

  void OnFrameDisplayed(uint32_t frame_id) override {
    on_frame_displayed_call_count_++;
    last_frame_id_ = frame_id;
    base::MessageLoop::current()->Quit();
  }

  mojo::Size initial_size_;
  scoped_ptr<GlHelper> gl_helper_;

  unsigned surface_id_changed_call_count_;
  mojo::SurfaceIdPtr last_surface_id_;
  unsigned on_frame_displayed_call_count_;
  uint32_t last_frame_id_;

  DISALLOW_COPY_AND_ASSIGN(GlHelperTest);
};

TEST_F(GlHelperTest, Basic) {
  gl_helper()->StartFrame();
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  scoped_ptr<unsigned char[]> bitmap(
      new unsigned char[initial_size().width * initial_size().height * 4]);
  memset(bitmap.get(), 123, initial_size().width * initial_size().height * 4);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, initial_size().width,
                  initial_size().height, GL_RGBA, GL_UNSIGNED_BYTE,
                  bitmap.get());
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  uint32_t frame_id = gl_helper()->EndFrame();

  // We should first get |OnSurfaceIdChanged()|.
  base::MessageLoop::current()->Run();
  EXPECT_EQ(1u, surface_id_changed_call_count());
  EXPECT_EQ(0u, on_frame_displayed_call_count());
  EXPECT_TRUE(last_surface_id());
  EXPECT_NE(last_surface_id()->local, 0u);
  EXPECT_NE(last_surface_id()->id_namespace, 0u);

  // Then we should get |OnFrameDisplayed()|.
  base::MessageLoop::current()->Run();
  EXPECT_EQ(1u, surface_id_changed_call_count());
  EXPECT_EQ(1u, on_frame_displayed_call_count());
  EXPECT_EQ(frame_id, last_frame_id());
}

TEST_F(GlHelperTest, SetSurfaceSizeAndMakeCurrent) {
  // It only creates surfaces lazily, so we have to start/end a frame.
  gl_helper()->StartFrame();
  GLuint frame_id = gl_helper()->EndFrame();
  base::MessageLoop::current()->Run();
  base::MessageLoop::current()->Run();
  EXPECT_EQ(1u, surface_id_changed_call_count());
  mojo::SurfaceId surface_id = *last_surface_id();
  EXPECT_EQ(1u, on_frame_displayed_call_count());
  EXPECT_EQ(frame_id, last_frame_id());

  // Set a new surface size.
  mojo::Size new_size;
  new_size.width = initial_size().width * 2;
  new_size.height = initial_size().height + 50;
  gl_helper()->SetSurfaceSize(new_size);

  gl_helper()->StartFrame();
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  GLuint frame_texture = gl_helper()->GetFrameTexture();
  EXPECT_NE(frame_texture, 0u);

  // Binding it should be OK.
  glBindTexture(GL_TEXTURE_2D, frame_texture);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  scoped_ptr<unsigned char[]> bitmap(
      new unsigned char[new_size.width * new_size.height * 4]);
  memset(bitmap.get(), 123, new_size.width * new_size.height * 4);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, new_size.width, new_size.height,
                  GL_RGBA, GL_UNSIGNED_BYTE, bitmap.get());
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  frame_id = gl_helper()->EndFrame();
  EXPECT_NE(frame_id, last_frame_id());

  // We should get another |OnSurfaceIdChanged()|.
  base::MessageLoop::current()->Run();
  EXPECT_EQ(2u, surface_id_changed_call_count());
  EXPECT_EQ(1u, on_frame_displayed_call_count());
  EXPECT_TRUE(last_surface_id());
  // Only the local part of the surface ID should have changed.
  EXPECT_NE(last_surface_id()->local, surface_id.local);
  EXPECT_EQ(surface_id.id_namespace, last_surface_id()->id_namespace);

  // Then we should get another |OnFrameDisplayed()|.
  base::MessageLoop::current()->Run();
  EXPECT_EQ(2u, surface_id_changed_call_count());
  EXPECT_EQ(2u, on_frame_displayed_call_count());
  EXPECT_EQ(frame_id, last_frame_id());

  // An out-of-the-blue |MakeCurrent()| should work.
  gl_helper()->MakeCurrent();
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
}

}  // namespace
