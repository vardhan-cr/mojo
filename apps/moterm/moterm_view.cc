// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/moterm/moterm_view.h"

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <algorithm>
#include <string>

#include "apps/moterm/key_util.h"
#include "base/logging.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/services/files/public/interfaces/file.mojom.h"
#include "mojo/services/files/public/interfaces/types.mojom.h"
#include "mojo/services/input_events/public/interfaces/input_event_constants.mojom.h"
#include "mojo/services/input_events/public/interfaces/input_events.mojom.h"
#include "mojo/services/terminal/public/interfaces/terminal_client.mojom.h"
#include "skia/ext/refptr.h"
#include "third_party/dejavu-fonts-ttf-2.34/kDejaVuSansMonoRegular.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkStream.h"
#include "third_party/skia/include/core/SkXfermode.h"

namespace {

const GLint kTextureFormat =
    (kN32_SkColorType == kRGBA_8888_SkColorType) ? GL_RGBA : GL_BGRA_EXT;

mojo::Size RectToSize(const mojo::Rect& rect) {
  mojo::Size size;
  size.width = rect.width;
  size.height = rect.height;
  return size;
}

}  // namespace

MotermView::MotermView(
    mojo::Shell* shell,
    mojo::View* view,
    mojo::InterfaceRequest<mojo::ServiceProvider> service_provider_request)
    : view_(view),
      gl_helper_(this,
                 shell,
                 kTextureFormat,
                 false,
                 RectToSize(view->bounds())),
      model_(MotermModel::Size(240, 160), MotermModel::Size(24, 80), this),
      frame_pending_(false),
      force_next_draw_(false),
      ascent_(0),
      line_height_(0),
      advance_width_(0),
      keypad_application_mode_(false) {
  // TODO(vtl): |service_provider_impl_|'s ctor doesn't like an invalid request,
  // so we have to conditionally, explicitly bind.
  if (service_provider_request.is_pending()) {
    service_provider_impl_.Bind(service_provider_request.Pass());
    service_provider_impl_.AddService<mojo::terminal::Terminal>(this);
  }

  regular_typeface_ = skia::AdoptRef(SkTypeface::CreateFromStream(
      new SkMemoryStream(font_data::kDejaVuSansMonoRegular.data,
                         font_data::kDejaVuSansMonoRegular.size)));

  // TODO(vtl): This duplicates some code.
  SkPaint fg_paint;
  fg_paint.setTypeface(regular_typeface_.get());
  fg_paint.setTextSize(16);
  // Figure out appropriate metrics.
  SkPaint::FontMetrics fm = {};
  fg_paint.getFontMetrics(&fm);
  ascent_ = static_cast<int>(ceilf(-fm.fAscent));
  line_height_ = ascent_ + static_cast<int>(ceilf(fm.fDescent + fm.fLeading));
  DCHECK_GT(line_height_, 0);
  // To figure out the advance width, measure an X. Better hope the font is
  // monospace.
  advance_width_ = static_cast<int>(ceilf(fg_paint.measureText("X", 1)));
  DCHECK_GT(advance_width_, 0);

  view_->AddObserver(this);

  // Force an initial draw.
  Draw(true);
}

MotermView::~MotermView() {
  if (driver_)
    driver_->Detach();
}

void MotermView::OnViewDestroyed(mojo::View* view) {
  DCHECK_EQ(view, view_);
  view_->RemoveObserver(this);
  delete this;
}

void MotermView::OnViewBoundsChanged(mojo::View* view,
                                     const mojo::Rect& old_bounds,
                                     const mojo::Rect& new_bounds) {
  DCHECK_EQ(view, view_);
  gl_helper_.SetSurfaceSize(RectToSize(view_->bounds()));
  bitmap_device_.clear();
  Draw(true);
}

void MotermView::OnViewInputEvent(mojo::View* view,
                                  const mojo::EventPtr& event) {
  if (event->action == mojo::EventType::KEY_PRESSED)
    OnKeyPressed(event);
}

void MotermView::OnSurfaceIdChanged(mojo::SurfaceIdPtr surface_id) {
  view_->SetSurfaceId(surface_id.Pass());
}

void MotermView::OnContextLost() {
  // TODO(vtl): We'll need to force a draw when we regain a context.
}

void MotermView::OnFrameDisplayed(uint32_t frame_id) {
  DCHECK(frame_pending_);
  frame_pending_ = false;
  Draw(false);
}

void MotermView::OnResponse(const void* buf, size_t size) {
  if (driver_)
    driver_->SendData(buf, size);
}

void MotermView::OnSetKeypadMode(bool application_mode) {
  keypad_application_mode_ = application_mode;
}

void MotermView::OnDataReceived(const void* bytes, size_t num_bytes) {
  model_.ProcessInput(bytes, num_bytes, &model_state_changes_);
  Draw(false);
}

void MotermView::OnClosed() {
  DCHECK(driver_);
  driver_->Detach();
  driver_.reset();

  OnDestroyed();
}

void MotermView::OnDestroyed() {
  DCHECK(!driver_);
  if (!on_closed_callback_.is_null()) {
    mojo::Closure callback;
    std::swap(callback, on_closed_callback_);
    callback.Run();
  }
}

void MotermView::Create(
    mojo::ApplicationConnection* connection,
    mojo::InterfaceRequest<mojo::terminal::Terminal> request) {
  terminal_bindings_.AddBinding(this, request.Pass());
}

void MotermView::Connect(
    mojo::InterfaceRequest<mojo::files::File> terminal_file,
    bool force,
    const ConnectCallback& callback) {
  if (driver_) {
    // We already have a connection.
    if (force) {
      OnClosed();
    } else {
      // TODO(vtl): Is this error code right?
      callback.Run(mojo::files::Error::UNAVAILABLE);
      return;
    }
  }

  driver_ = MotermDriver::Create(this, terminal_file.Pass());
  DCHECK(on_closed_callback_.is_null());
  on_closed_callback_ = [callback] { callback.Run(mojo::files::Error::OK); };
}

void MotermView::ConnectToClient(
    mojo::terminal::TerminalClientPtr terminal_client,
    bool force,
    const ConnectToClientCallback& callback) {
  if (driver_) {
    // We already have a connection.
    if (force) {
      OnClosed();
    } else {
      // TODO(vtl): Is this error code right?
      callback.Run(mojo::files::Error::UNAVAILABLE);
      return;
    }
  }

  mojo::files::FilePtr file;
  driver_ = MotermDriver::Create(this, GetProxy(&file));
  terminal_client->ConnectToTerminal(file.Pass());
  DCHECK(on_closed_callback_.is_null());
  on_closed_callback_ = [callback] { callback.Run(mojo::files::Error::OK); };
}

void MotermView::GetSize(const GetSizeCallback& callback) {
  MotermModel::Size size = model_.GetSize();
  callback.Run(mojo::files::Error::OK, size.rows, size.columns);
}

void MotermView::SetSize(uint32_t rows,
                         uint32_t columns,
                         bool reset,
                         const SetSizeCallback& callback) {
  if (!rows) {
    rows = std::max(1u, std::min(MotermModel::kMaxRows,
                                 static_cast<uint32_t>(view_->bounds().height) /
                                     line_height_));
  }
  if (!columns) {
    columns =
        std::max(1u, std::min(MotermModel::kMaxColumns,
                              static_cast<uint32_t>(view_->bounds().width) /
                                  advance_width_));
  }

  model_.SetSize(MotermModel::Size(rows, columns), reset);
  callback.Run(mojo::files::Error::OK, rows, columns);

  Draw(false);
}

void MotermView::Draw(bool force) {
  // TODO(vtl): See TODO above |frame_pending_| in the class declaration.
  if (frame_pending_) {
    force_next_draw_ |= force;
    return;
  }

  force |= force_next_draw_;
  force_next_draw_ = false;

  if (!force && !model_state_changes_.IsDirty())
    return;

  // TODO(vtl): If |!force|, draw only the dirty region(s)?
  model_state_changes_.Reset();

  int32_t width = view_->bounds().width;
  int32_t height = view_->bounds().height;
  DCHECK_GT(width, 0);
  DCHECK_GT(height, 0);

  if (!bitmap_device_) {
    bitmap_device_ = skia::AdoptRef(SkBitmapDevice::Create(SkImageInfo::Make(
        width, height, kN32_SkColorType, kOpaque_SkAlphaType)));
  }

  SkCanvas canvas(bitmap_device_.get());
  canvas.clear(SK_ColorBLACK);

  SkPaint bg_paint;
  bg_paint.setStyle(SkPaint::kFill_Style);

  SkPaint fg_paint;
  fg_paint.setTypeface(regular_typeface_.get());
  fg_paint.setTextSize(16);
  fg_paint.setTextEncoding(SkPaint::kUTF32_TextEncoding);

  MotermModel::Size size = model_.GetSize();
  int y = 0;
  for (unsigned i = 0; i < size.rows; i++, y += line_height_) {
    int x = 0;
    for (unsigned j = 0; j < size.columns; j++, x += advance_width_) {
      MotermModel::CharacterInfo ch =
          model_.GetCharacterInfoAt(MotermModel::Position(i, j));

      // Paint the background.
      bg_paint.setColor(SkColorSetRGB(ch.background_color.red,
                                      ch.background_color.green,
                                      ch.background_color.blue));
      canvas.drawRect(SkRect::MakeXYWH(x, y, advance_width_, line_height_),
                      bg_paint);

      // Paint the foreground.
      if (ch.code_point) {
        uint32_t flags = SkPaint::kAntiAlias_Flag;
        // TODO(vtl): Use real bold font?
        if ((ch.attributes & MotermModel::kAttributesBold))
          flags |= SkPaint::kFakeBoldText_Flag;
        if ((ch.attributes & MotermModel::kAttributesUnderline))
          flags |= SkPaint::kUnderlineText_Flag;
        // TODO(vtl): Handle blink, because that's awesome.
        fg_paint.setFlags(flags);
        fg_paint.setColor(SkColorSetRGB(ch.foreground_color.red,
                                        ch.foreground_color.green,
                                        ch.foreground_color.blue));

        canvas.drawText(&ch.code_point, sizeof(ch.code_point), x, y + ascent_,
                        fg_paint);
      }
    }
  }

  if (model_.GetCursorVisibility()) {
    // Draw the cursor.
    MotermModel::Position cursor_pos = model_.GetCursorPosition();
    // Reuse the background paint, but don't just paint over.
    // TODO(vtl): Consider doing other things. Maybe make it blink, to be extra
    // annoying.
    // TODO(vtl): Maybe vary how we draw the cursor, depending on if we're
    // focused and/or active.
    bg_paint.setColor(SK_ColorWHITE);
    bg_paint.setXfermodeMode(SkXfermode::kDifference_Mode);
    canvas.drawRect(SkRect::MakeXYWH(cursor_pos.column * advance_width_,
                                     cursor_pos.row * line_height_,
                                     advance_width_, line_height_),
                    bg_paint);
  }

  canvas.flush();

  const SkBitmap& bitmap(bitmap_device_->accessBitmap(false));
  // TODO(vtl): Do we need really need to lock/unlock pixels?
  SkAutoLockPixels pixel_locker(bitmap);

  gl_helper_.StartFrame();
  // (The texture is already bound.)
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, kTextureFormat,
                  GL_UNSIGNED_BYTE, bitmap.getPixels());
  gl_helper_.EndFrame();
  frame_pending_ = true;
}

void MotermView::OnKeyPressed(const mojo::EventPtr& key_event) {
  std::string input_sequence =
      GetInputSequenceForKeyPressedEvent(*key_event, keypad_application_mode_);
  if (input_sequence.empty())
    return;

  if (driver_)
    driver_->SendData(input_sequence.data(), input_sequence.size());
}
