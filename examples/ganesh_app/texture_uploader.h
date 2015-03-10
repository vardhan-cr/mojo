// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_GANESH_APP_TEXTURE_UPLOADER_H_
#define EXAMPLES_GANESH_APP_TEXTURE_UPLOADER_H_

#include "base/containers/hash_tables.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "mojo/gpu/gl_context.h"
#include "mojo/gpu/gl_texture.h"
#include "mojo/services/geometry/public/interfaces/geometry.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surface_id.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surfaces.mojom.h"

namespace mojo {
class Shell;
}

namespace examples {

class TextureUploader : public mojo::ResourceReturner,
                        public mojo::GLContext::Observer {
 public:
  class Client {
   public:
    virtual void OnSurfaceIdAvailable(mojo::SurfaceIdPtr surface_id) = 0;

   protected:
    virtual ~Client();
  };

  TextureUploader(Client* client,
                  mojo::Shell* shell,
                  base::WeakPtr<mojo::GLContext> context);
  ~TextureUploader() override;

  void Upload(scoped_ptr<mojo::GLTexture> texture);

 private:
  // mojo::GLContext::Observer
  void OnContextLost() override;

  // mojo::ResourceReturner
  void ReturnResources(
      mojo::Array<mojo::ReturnedResourcePtr> resources) override;

  void SetIdNamespace(uint32_t id_namespace);
  void EnsureSurfaceForSize(const mojo::Size& size);
  void SendFullyQualifiedID();

  Client* client_;
  base::WeakPtr<mojo::GLContext> context_;
  scoped_ptr<mojo::GLTexture> pending_upload_;
  mojo::SurfacePtr surface_;
  mojo::Size surface_size_;
  uint32_t next_resource_id_;
  uint32_t id_namespace_;
  uint32_t local_id_;
  base::hash_map<uint32_t, mojo::GLTexture*> resource_to_texture_map_;
  mojo::Binding<mojo::ResourceReturner> returner_binding_;

  base::WeakPtrFactory<TextureUploader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(TextureUploader);
};

}  // namespace examples

#endif  // EXAMPLES_GANESH_APP_TEXTURE_UPLOADER_H_
