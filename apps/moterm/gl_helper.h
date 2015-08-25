// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_MOTERM_GL_HELPER_H_
#define APPS_MOTERM_GL_HELPER_H_

#include <GLES2/gl2.h>
#include <MGL/mgl_types.h>

#include <deque>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/services/geometry/public/interfaces/geometry.mojom.h"
#include "mojo/services/gpu/public/interfaces/gpu.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surface_id.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surfaces.mojom.h"

namespace mojo {
class Shell;
}

// A class that helps with drawing surfaces using GL.
class GlHelper : public mojo::ResourceReturner {
 public:
  class Client {
   public:
    // Called when the surface ID changes (including the first time it becomes
    // available).
    virtual void OnSurfaceIdChanged(mojo::SurfaceIdPtr surface_id) = 0;

    // Called when the GL context is lost.
    virtual void OnContextLost() = 0;

    // Called when the frame with ID |frame_id| (from |EndFrame()|) is drawn for
    // the first time. Use this to rate-limit draws.
    virtual void OnFrameDisplayed(uint32_t frame_id) = 0;
  };

  // Both |client| and |shell| must outlive us. |texture_format| is the texture
  // format to use, e.g., |GL_RGBA| or |GL_BGRA_EXT|. |flipped| means that the
  // texture is in the usual GL orientation (origin at lower-left). This object
  // will create a surface (of initial size |initial_size|), and call the
  // client's |OnSurfaceIdChanged()| when it has a surface ID for it (which the
  // client can then provide to its |View|).
  GlHelper(Client* client,
           mojo::Shell* shell,
           GLint texture_format,
           bool flipped,
           const mojo::Size& initial_size);
  ~GlHelper() override;

  // Sets the size of the surface. (The surface will only be created at this
  // size lazily at the next |StartFrame()|.)
  void SetSurfaceSize(const mojo::Size& surface_size);

  // Ensures that a GL context is available and makes it current. (Note that
  // this is automatically called by |StartFrame()|, so this is mostly useful
  // for creating GL resources outside |StartFrame()|/|EndFrame()|.
  void MakeCurrent();

  // Starts a frame; makes an appropriate GL context current, and binds a
  // texture of the current size (creating it if necessary), returning its ID.
  // Between |StartFrame()| and |EndFrame()|, the run loop must not be run and
  // no other methods of this object other than |MakeCurrent()| and
  // |GetFrameTexture()| may be called. This object must not be destroyed after
  // |StartFrame()| before calling |EndFrame()|.
  void StartFrame();
  // Completes the current frame (started using |StartFrame()|). Before calling
  // this, the frame's texture must be bound and the GL context must be current.
  // (This is the default state after |StartFrame()|, so nothing has to be done
  // unless another texture is bound or another GL context is made current,
  // respectively.) Returns an ID for this frame (which will be given to the
  // client on |OnFrameDisplayed()|).
  uint32_t EndFrame();

  // Gets the texture that will be used for the current frame, e.g., so that it
  // may be (re)bound. Only valid between |StartFrame()| and |EndFrame()|. (Note
  // that this texture is automatically bound by |StartFrame()|, so this is
  // typically only needed if another texture is bound.)
  GLuint GetFrameTexture();

 private:
  struct TextureInfo {
    TextureInfo(uint32_t resource_id, GLuint texture, const mojo::Size& size)
        : resource_id(resource_id), texture(texture), size(size) {}

    // Only interesting if it's pending return.
    uint32_t resource_id;
    GLuint texture;
    mojo::Size size;
  };

  // |mojo::ResourceReturner|:
  void ReturnResources(
      mojo::Array<mojo::ReturnedResourcePtr> resources) override;

  // Ensures that we have a GL context and that it is current.
  void EnsureContext();

  // Ensures that we have a surface of the appropriate size. This should only be
  // called with a valid GL context which is current (e.g., after calling
  // |EnsureContext()|).
  void EnsureSurface();

  // Calls the client's |OnSurfaceIdChanged()| if appropriate (both
  // |id_namespace_| and |local_id_| must be set).
  void CallOnSurfaceIdChanged();

  // Texture queue functions. (For all functions, |mgl_context_| should be
  // current.)
  // Gets and binds a texture of size |current_surface_size_|.
  TextureInfo GetTexture();
  // Returns a texture to the queue (or deletes it, if it's not of the right
  // size or there are already enough textures in the queue).
  void ReturnTexture(const TextureInfo& texture_info);
  // Clears all textures (i.e., calls |glDeleteTextures()| on all textures in
  // the texture queue).
  void ClearTextures();

  // Callbacks:

  // Callback for |GetIdNamespace()|:
  void GetIdNamespaceCallback(uint32_t id_namespace);

  // "Callback" for |MojoGLES2CreateContext()|:
  static void OnContextLostThunk(void* self);
  void OnContextLost();

  // Callback for |SubmitFrame()|:
  void SubmitFrameCallback(uint32_t frame_id);

  Client* const client_;
  const GLint texture_format_;
  const bool flipped_;

  mojo::GpuPtr gpu_;
  mojo::SurfacePtr surface_;
  mojo::Binding<mojo::ResourceReturner> returner_binding_;

  // The size for the surface at the next |StartFrame()|.
  mojo::Size next_surface_size_;

  MGLContext mgl_context_;

  std::deque<TextureInfo> textures_;

  uint32_t next_frame_id_;
  // The texture that'll be used to draw the current frame. Only valid (nonzero)
  // between |StartFrame()| and |EndFrame()|.
  GLuint frame_texture_;

  uint32_t id_namespace_;
  uint32_t local_id_;
  // If |local_id_| is nonzero, there's currently a surface, in which case this
  // is its size.
  mojo::Size current_surface_size_;

  uint32_t next_resource_id_;
  std::vector<TextureInfo> textures_pending_return_;

  base::WeakPtrFactory<GlHelper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(GlHelper);
};

#endif  // APPS_MOTERM_GL_HELPER_H_
