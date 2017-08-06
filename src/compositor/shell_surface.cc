#include "compositor/shell_surface.h"

#include "compositor/buffer.h"
#include "compositor/surface.h"
#include "wm/window_manager.h"

namespace naive {

ShellSurface::ShellSurface(Surface* surface)
    : surface_(surface), window_(surface->window()) {
  window_->SetShellSurface(this);
  TRACE("add shell %p as observer to %p", this, surface_);
  surface_->AddSurfaceObserver(this);
}

ShellSurface::~ShellSurface() {
  TRACE("%p", this);
  if (window_)
    wm::WindowManager::Get()->RemoveWindow(window_);
  if (surface_)
    surface_->RemoveSurfaceObserver(this);
}

void ShellSurface::Configure(int32_t width, int32_t height) {
  if (!window_ || window_->is_transient())
    return;
  in_configure_ = true;
  configure_callback_(width, height);
}

void ShellSurface::Close() {
  if (!window_)
    return;
  close_callback_();
  close_callback_ = []() {};
}

void ShellSurface::SetPosition(int32_t x, int32_t y) {
  pending_state_.geometry.x_ = x;
  pending_state_.geometry.y_ = y;
}

void ShellSurface::SetGeometry(const base::geometry::Rect& rect) {
  TRACE("%d %d %d %d", rect.x(), rect.y(), rect.width(), rect.height());
  pending_state_.geometry = rect;
}

void ShellSurface::SetVisibleRegion(const base::geometry::Rect& rect) {
  TRACE("%d %d %d %d", rect.x(), rect.y(), rect.width(), rect.height());
  pending_state_.visible_region = rect;
}

void ShellSurface::Move() {
  if (!window_)
    return;

  window_->BeginMove();
}

void ShellSurface::AcknowledgeConfigure(uint32_t serial) {
  in_configure_ = false;
  // TODO: need to implement this?
  NOTIMPLEMENTED();
}

void ShellSurface::OnCommit(Surface* committed_surface) {
  if (!window_)
    return;

  if (window_->IsManaged() && (!surface_->committed_buffer() ||
                               !surface_->committed_buffer()->data())) {
    // TODO: How to anounce size?
    return;
  }

  state_ = pending_state_;
  window_->PushProperty(state_.geometry, state_.visible_region);
  window_->MaybeMakeTopLevel();
}

void ShellSurface::OnSurfaceDestroyed(Surface* surface) {
  if (surface == surface_) {
    wm::WindowManager::Get()->RemoveWindow(window_);
    surface_ = nullptr;
    window_ = nullptr;
  }
}

}  // namespace naive
