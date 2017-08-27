#include "wm/window_impl/window_impl_wayland.h"

#include <cassert>

#include "base/logging.h"
#include "compositor/buffer.h"
#include "compositor/shell_surface.h"
#include "compositor/surface.h"

namespace naive {
namespace wm {

WindowImplWayland::WindowImplWayland(Surface* surface) : surface_(surface) {}

WindowImplWayland::~WindowImplWayland() {}

void WindowImplWayland::set_shell_surface(ShellSurface* shell_surface) {
  shell_surface_ = shell_surface;
}

void WindowImplWayland::SurfaceUngrab() {
  assert(shell_surface_);
  shell_surface_->Ungrab();
}

void WindowImplWayland::SurfaceClose() {
  if (!shell_surface_) {
    LOG_ERROR << " only shell surface can call close." << std::endl;
    return;
  }
  shell_surface_->Close();
}

void WindowImplWayland::NotifyFrameRendered() {
  assert(surface_);
  surface_->RunSurfaceCallback();
}

void WindowImplWayland::AddDamage(const base::geometry::Rect& rect) {
  assert(surface_);
  surface_->ForceDamage(rect);
}

bool WindowImplWayland::CanResize() {
  return !!shell_surface_;
}

void WindowImplWayland::Configure(int32_t width, int32_t height) {
  if (!shell_surface_) {
    LOG_ERROR << " only shell surface can call configure." << std::endl;
    return;
  }

  shell_surface_->Configure(width, height);
}

void WindowImplWayland::TakeFocus() {
  if (!shell_surface_) {
    LOG_ERROR << " only shell surface can call take focus." << std::endl;
    return;
  }

  auto geometry = shell_surface_->window()->geometry();
  shell_surface_->Configure(geometry.width(), geometry.height());
  shell_surface_->Activate();
}

void WindowImplWayland::ForceCommit() {
  assert(surface_);
  surface_->force_commit();
}

bool WindowImplWayland::HasCommit() {
  assert(surface_);
  return surface_->has_commit();
}

Region WindowImplWayland::DamagedRegion() {
  assert(surface_);
  return surface_->damaged_regoin();
}

compositor::DrawQuad WindowImplWayland::GetQuad() {
  assert(surface_);
  if (!surface_->committed_buffer() || !surface_->committed_buffer()->data())
    return compositor::DrawQuad();

  return compositor::DrawQuad(surface_->committed_buffer());
}

void WindowImplWayland::ClearCommit() {
  assert(surface_);
  surface_->clear_commit();
}

void WindowImplWayland::ClearDamage() {
  assert(surface_);
  surface_->clear_damage();
}

int32_t WindowImplWayland::GetScale() {
  assert(surface_);
  return surface_->buffer_scale();
}

}  // namespace wm
}  // namespace naive
