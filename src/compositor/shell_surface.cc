#include "compositor/shell_surface.h"

#include "compositor/buffer.h"
#include "compositor/surface.h"
#include "wm/window_manager.h"

namespace naive {

ShellSurface::ShellSurface(Surface* surface) :
    surface_(surface), window_(surface->window()) {
  window_->SetShellSurface(this);
  surface_->AddSurfaceObserver(this);
}

ShellSurface::~ShellSurface() {
  wm::WindowManager::Get()->RemoveWindow(window_);
  surface_->RemoveSurfaceObserver(this);
}

void ShellSurface::Configure(int32_t width, int32_t height) {
  in_configure_ = true;
  configure_callback_(width, height);
}

void ShellSurface::SetGeometry(const base::geometry::Rect &rect) {
  window_->SetGeometry(rect);
}

void ShellSurface::Move() {
  window_->BeginMove();
}

void ShellSurface::AcknowledgeConfigure(uint32_t serial) {
  in_configure_ = false;
  // TODO: need to implement this?
  NOTIMPLEMENTED();
}

void ShellSurface::OnCommit() {
  if (window_->IsManaged() && (!surface_->committed_buffer()
      || !surface_->committed_buffer()->data())) {
    // TODO: How to anounce size?
    TRACE("%d %d", window_->wm_width(), window_->wm_height());
    Configure(window_->wm_width(), window_->wm_height());
    return;
  }
}

}  // namespace naive
