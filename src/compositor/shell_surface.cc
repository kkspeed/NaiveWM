#include "compositor/shell_surface.h"

#include "compositor/buffer.h"
#include "compositor/surface.h"
#include "wm/window_manager.h"

namespace naive {

ShellSurface::ShellSurface(Surface* surface)
    : surface_(surface), window_(surface->window()) {
  window_->SetShellSurface(this);
  surface_->AddSurfaceObserver(this);
}

ShellSurface::~ShellSurface() {
  if (!window_) return;
  wm::WindowManager::Get()->RemoveWindow(window_);
  surface_->RemoveSurfaceObserver(this);
}

void ShellSurface::Configure(int32_t width, int32_t height) {
  if (!window_ || window_->is_popup() || window_->is_transient())
    return;
  in_configure_ = true;
  configure_callback_(width, height);
}

void ShellSurface::Close() {
  close_callback_();
}

void ShellSurface::SetGeometry(const base::geometry::Rect& rect) {
  TRACE("%d %d %d %d", rect.x(), rect.y(), rect.width(), rect.height());
  if (!window_)
    return;
  window_->SetGeometry(rect);
}

void ShellSurface::SetVisibleRegion(const base::geometry::Rect& rect) {
  TRACE("%d %d %d %d", rect.x(), rect.y(), rect.width(), rect.height());
  if (!window_)
    return;
  window_->SetVisibleRegion(rect);
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

void ShellSurface::OnCommit() {
  if (!window_)
    return;

  if (window_->IsManaged() && (!surface_->committed_buffer() ||
                               !surface_->committed_buffer()->data())) {
    // TODO: How to anounce size?
    return;
  }
}

void ShellSurface::OnSurfaceDestroyed(Surface* surface) {
  if (surface == surface_) {
    wm::WindowManager::Get()->RemoveWindow(window_);
    surface_ = nullptr;
    window_ = nullptr;
  }
}

}  // namespace naive
