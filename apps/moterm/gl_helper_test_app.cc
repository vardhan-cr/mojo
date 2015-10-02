// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a test application for gl_helper.*, which draws directly to a native
// viewport (without using the view manager).

#include <GLES2/gl2.h>
#include <math.h>

#include "apps/moterm/gl_helper.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/services/geometry/public/interfaces/geometry.mojom.h"
#include "mojo/services/gpu/public/interfaces/context_provider.mojom.h"
#include "mojo/services/native_viewport/public/interfaces/native_viewport.mojom.h"
#include "mojo/services/surfaces/public/interfaces/display.mojom.h"
#include "mojo/services/surfaces/public/interfaces/quads.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surfaces.mojom.h"

namespace {

GLuint LoadShader(GLenum type, const char* shader_source) {
  GLuint shader = glCreateShader(type);
  CHECK(shader);
  glShaderSource(shader, 1, &shader_source, nullptr);
  glCompileShader(shader);
  GLint compiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  CHECK(compiled);
  return shader;
}

class GlHelperExampleApp : public mojo::ApplicationDelegate,
                           public GlHelper::Client {
 public:
  GlHelperExampleApp()
      : application_impl_(),
        program_(0),
        transform_location_(0),
        scale_location_(0),
        framebuffer_(0),
        vertex_buffer_(0),
        draw_number_(0) {}
  ~GlHelperExampleApp() override {}

 private:
  void OnViewportMetricsReceived(mojo::ViewportMetricsPtr metrics) {
    DVLOG(1) << "Viewport metrics received: size = " << metrics->size->width
             << "x" << metrics->size->height
             << ", device_pixel_ratio = " << metrics->device_pixel_ratio;

    if (metrics->size->width != viewport_size_.width ||
        metrics->size->height != viewport_size_.height) {
      viewport_size_ = *metrics->size;
      gl_helper_->SetSurfaceSize(viewport_size_);
    }

    native_viewport_->RequestMetrics(
        base::Bind(&GlHelperExampleApp::OnViewportMetricsReceived,
                   base::Unretained(this)));
  }

  void InitGlState() {
    static const char vertex_shader_source[] =
        "attribute vec4 position;                        \n"
        "uniform mat4 transform;                         \n"
        "uniform vec4 scale;                             \n"
        "void main() {                                   \n"
        "  gl_Position = (position * transform) * scale; \n"
        "}                                               \n";
    static const char fragment_shader_source[] =
        "precision mediump float;                   \n"
        "void main() {                              \n"
        "  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); \n"
        "}                                          \n";
    static const float vertices[] = {
        0.f, 0.5f, -sqrtf(5.f) / 4.f, -0.25f, sqrtf(5.f) / 4.f, -0.25f};
    GLuint vertex_shader = LoadShader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragment_shader =
        LoadShader(GL_FRAGMENT_SHADER, fragment_shader_source);
    program_ = glCreateProgram();
    CHECK(program_);
    glAttachShader(program_, vertex_shader);
    glAttachShader(program_, fragment_shader);
    glBindAttribLocation(program_, kPositionLocation, "position");
    glLinkProgram(program_);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    GLint linked = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    CHECK(linked);
    transform_location_ = glGetUniformLocation(program_, "transform");
    CHECK_NE(transform_location_, -1);
    scale_location_ = glGetUniformLocation(program_, "scale");
    CHECK_NE(scale_location_, -1);

    glGenFramebuffers(1, &framebuffer_);
    CHECK(framebuffer_);

    glGenBuffers(1, &vertex_buffer_);
    CHECK(vertex_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glClearColor(0.f, 1.f, 0.f, 1.f);
  }

  void Draw() {
    DCHECK(gl_helper_);

    gl_helper_->StartFrame();

    if (!program_)
      InitGlState();

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           gl_helper_->GetFrameTexture(), 0);
    glViewport(0, 0, viewport_size_.width, viewport_size_.height);

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program_);

    float angle = static_cast<float>(draw_number_ % 360) * M_PI / 180;
    float transform[4][4] = {{cosf(angle), sinf(angle), 0.f, 0.f},
                             {-sinf(angle), cosf(angle), 0.f, 0.f},
                             {0.f, 0.f, 1.f, 0.f},
                             {0.f, 0.f, 0.f, 1.f}};
    glUniformMatrix4fv(transform_location_, 1, GL_FALSE, &transform[0][0]);
    if (viewport_size_.width >= viewport_size_.height) {
      glUniform4f(scale_location_, static_cast<float>(viewport_size_.height) /
                                       static_cast<float>(viewport_size_.width),
                  1.f, 1.f, 1.f);
    } else {
      glUniform4f(scale_location_, 1.f,
                  static_cast<float>(viewport_size_.width) /
                      static_cast<float>(viewport_size_.height),
                  1.f, 1.f);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glVertexAttribPointer(kPositionLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(kPositionLocation);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    gl_helper_->EndFrame();

    draw_number_++;
  }

  void SubmitFrame() {
    DCHECK(surface_id_.local);
    DCHECK(surface_id_.id_namespace);

    mojo::FramePtr frame = mojo::Frame::New();
    frame->resources = mojo::Array<mojo::TransferableResourcePtr>::New(0);
    frame->passes = mojo::Array<mojo::PassPtr>::New(1);
    mojo::PassPtr pass = mojo::Pass::New();
    pass->id = 0;
    mojo::Rect viewport_rect;
    viewport_rect.width = viewport_size_.width;
    viewport_rect.height = viewport_size_.height;
    pass->output_rect = viewport_rect.Clone();
    pass->damage_rect = viewport_rect.Clone();
    mojo::Transform identity_transform;
    identity_transform.matrix = mojo::Array<float>::New(16);
    for (size_t i = 0; i < 4; i++) {
      for (size_t j = 0; j < 4; j++)
        identity_transform.matrix[4 * i + j] = (i == j);
    }
    pass->transform_to_root_target = identity_transform.Clone();
    pass->has_transparent_background = false;
    pass->quads = mojo::Array<mojo::QuadPtr>::New(1);
    mojo::QuadPtr quad = mojo::Quad::New();
    quad->material = mojo::Material::SURFACE_CONTENT;
    quad->rect = viewport_rect.Clone();
    quad->opaque_rect = viewport_rect.Clone();
    quad->visible_rect = viewport_rect.Clone();
    quad->needs_blending = false;
    quad->shared_quad_state_index = 0;
    mojo::SurfaceQuadStatePtr surface_quad_state =
        mojo::SurfaceQuadState::New();
    surface_quad_state->surface = surface_id_.Clone();
    quad->surface_quad_state = surface_quad_state.Pass();

    pass->quads[0] = quad.Pass();
    pass->shared_quad_states = mojo::Array<mojo::SharedQuadStatePtr>::New(1);
    mojo::SharedQuadStatePtr shared_quad_state = mojo::SharedQuadState::New();
    shared_quad_state->content_to_target_transform = identity_transform.Clone();
    shared_quad_state->content_bounds = viewport_size_.Clone();
    shared_quad_state->visible_content_rect = viewport_rect.Clone();
    shared_quad_state->clip_rect = viewport_rect.Clone();
    shared_quad_state->is_clipped = false;
    shared_quad_state->opacity = 1;
    shared_quad_state->blend_mode = mojo::SkXfermode::kSrc_Mode;
    shared_quad_state->sorting_context_id = 0;
    pass->shared_quad_states[0] = shared_quad_state.Pass();
    frame->passes[0] = pass.Pass();

    DCHECK(display_);
    display_->SubmitFrame(frame.Pass(), mojo::Display::SubmitFrameCallback());
  }

  // |mojo::ApplicationDelegate|:
  void Initialize(mojo::ApplicationImpl* application_impl) override {
    DCHECK(!application_impl_);
    application_impl_ = application_impl;

    application_impl->ConnectToService("mojo:native_viewport_service",
                                       &native_viewport_);
    viewport_size_.width = 800;
    viewport_size_.height = 600;
    native_viewport_->Create(
        viewport_size_.Clone(), mojo::SurfaceConfiguration::New(),
        base::Bind(&GlHelperExampleApp::OnViewportMetricsReceived,
                   base::Unretained(this)));
    native_viewport_->Show();

    mojo::ContextProviderPtr context_provider;
    native_viewport_->GetContextProvider(GetProxy(&context_provider));

    mojo::DisplayFactoryPtr display_factory;
    application_impl_->ConnectToService("mojo:surfaces_service",
                                        &display_factory);
    display_factory->Create(context_provider.Pass(), nullptr,
                            GetProxy(&display_));

    gl_helper_.reset(new GlHelper(this, application_impl_->shell(), GL_RGBA,
                                  false, viewport_size_));
    Draw();
  }

  // |GlHelper::Client|:
  void OnSurfaceIdChanged(mojo::SurfaceIdPtr surface_id) override {
    surface_id_ = *surface_id;
    SubmitFrame();
  }

  void OnContextLost() override { CHECK(false) << "Oops, lost context."; }

  void OnFrameDisplayed(uint32_t frame_id) override { Draw(); }

  static const GLuint kPositionLocation = 0;

  mojo::ApplicationImpl* application_impl_;
  mojo::NativeViewportPtr native_viewport_;
  mojo::Size viewport_size_;
  mojo::DisplayPtr display_;
  scoped_ptr<GlHelper> gl_helper_;

  mojo::SurfaceId surface_id_;

  // GL-related state:
  GLuint program_;
  GLint transform_location_;
  GLint scale_location_;
  GLuint framebuffer_;
  GLuint vertex_buffer_;

  uint64_t draw_number_;

  DISALLOW_COPY_AND_ASSIGN(GlHelperExampleApp);
};

const GLuint GlHelperExampleApp::kPositionLocation;

}  // namespace

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new GlHelperExampleApp());
  return runner.Run(application_request);
}
