#include "compositor/shell_surface.h"

#include "compositor/buffer.h"
#include "compositor/surface.h"
#include "wm/window_manager.h"
#include "wm/window_impl.h"
#include "wm/window_impl/window_impl_wayland.h"

namespace naive {

ShellSurface::ShellSurface(Surface* surface)
    : surface_(surface), window_(surface->window()) {
  window_->SetShellSurface(this);
  TRACE("add shell %p as observer to %p", this, surface_);
  surface_->AddSurfaceObserver(this);
}

ShellSurface::~ShellSurface() {
  TRACE("%p", this);
  if (window_) {
    static_cast<wm::WindowImplWayland*>(window_->window_impl())
        ->set_shell_surface(nullptr);
    wm::WindowManager::Get()->RemoveWindow(window_);
  }
  if (surface_)
    surface_->RemoveSurfaceObserver(this);
  if (cached_window_state_ && cached_window_state_->parent_)
    cached_window_state_->parent_->surface()->RemoveSurfaceObserver(this);
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
    CachedWindowState();
    wm::WindowManager::Get()->RemoveWindow(window_);
    surface_ = nullptr;
    window_ = nullptr;
    return;
  }
  if (cached_window_state_ &&
      surface->window() == cached_window_state_->parent_) {
    cached_window_state_->parent_ = nullptr;
    return;
  }
}

void ShellSurface::RecoverWindowState(ShellSurface* other) {
  TRACE("this: %p, other: %p", this, other);
  if (cached_window_state_) {
    if (cached_window_state_->parent_)
      cached_window_state_->parent_->AddChild(other->window_);
    other->window_->override_border(cached_window_state_->has_border_,
                                    cached_window_state_->has_border_);
    other->window_->PushProperty(cached_window_state_->geometry_,
                                 cached_window_state_->geometry_);
    other->window_->set_transient(cached_window_state_->is_transient_);
    other->window_->set_popup(cached_window_state_->is_popup_);
    other->window_->WmSetPosition(cached_window_state_->wm_x_,
                                  cached_window_state_->wm_y_);
  }
}

void ShellSurface::CacheWindowState() {
  TRACE("window: %p, shell: %p", window_, this);
  if (!window_)
    return;
  cached_window_state_ = std::make_unique<CachedWindowState>();
  cached_window_state_->geometry_ = window_->geometry();
  cached_window_state_->is_popup_ = window_->is_popup();
  cached_window_state_->is_transient_ = window_->is_transient();
  cached_window_state_->wm_x_ = window_->wm_x();
  cached_window_state_->wm_y_ = window_->wm_y();
  // TODO: possibly needs to observe parent destruction.. otherwise, it
  // might hit NPE.
  cached_window_state_->parent_ = window_->parent();
  if (cached_window_state_->parent_)
    cached_window_state_->parent_->surface()->AddSurfaceObserver(this);
  cached_window_state_->has_border_ = window_->has_border();
}

}  // namespace naive
