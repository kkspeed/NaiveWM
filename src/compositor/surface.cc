#include "compositor/surface.h"

#include <algorithm>

#include "compositor/buffer.h"
#include "compositor/compositor.h"
#include "wayland/display_metrics.h"
#include "wm/window.h"

namespace naive {

Surface::Surface() : window_(std::make_unique<wm::Window>()) {
  window_->set_surface(this);
}

Surface::~Surface() {
  TRACE("resource %p, window %p", resource(), window());

  if (window()->parent())
    window()->parent()->surface()->Damage(window()->geometry());

  for (size_t i = 0; i < observers_.size(); i++) {
    auto* observer = observers_[i];
    LOG_ERROR << "surface " << this << " destroyed for " << observer
              << std::endl;
    observer->OnSurfaceDestroyed(this);
  }
}

void Surface::Attach(Buffer* buffer) {
  LOG_ERROR << "Surface::Attach " << buffer << " to window " << window()
            << std::endl;
  wayland::DisplayMetrics* metrics =
      compositor::Compositor::Get()->GetDisplayMetrics();
  if (buffer) {
    buffer->SetOwningSurface(this);
    window_->SetGeometry(base::geometry::Rect(
        window_->pending_geometry().x(), window_->pending_geometry().y(),
        buffer->width() / metrics->scale, buffer->height() / metrics->scale));
    Damage(window_->geometry());
  }
  pending_state_.buffer = buffer;
}

void Surface::SetFrameCallback(std::function<void()>* callback) {
  pending_state_.frame_callback = callback;
}

void Surface::Damage(const base::geometry::Rect& rect) {
  pending_state_.damaged_region.Union(rect);
}

void Surface::SetOpaqueRegion(const Region region) {
  pending_state_.opaque_region = region;
}

void Surface::SetInputRegion(const Region region) {
  pending_state_.input_region = region;
}

void Surface::Commit() {
  TRACE("window: %p, surface: %p", window(), this);
  if (pending_state_.buffer != state_.buffer && state_.buffer)
    state_.buffer->Release();
  state_ = pending_state_;
  if (!state_.buffer || !state_.buffer->data())
    TRACE("window: %p does not have buffer", window());
  has_commit_ = true;

  pending_state_.frame_callback = nullptr;
  pending_state_.buffer = nullptr;
  pending_state_.damaged_region = Region::Empty();

  // TODO: Can't use iterator due to possible iterator invalidation...
  // needs revisit
  for (size_t i = 0; i < observers_.size(); i++) {
    auto* observer = observers_[i];
    TRACE("notifying %p for commit", observer);
    observer->OnCommit();
  }
}

void Surface::ForceDamage(base::geometry::Rect rect) {
  TRACE("force damage %s on %p", rect.ToString().c_str(), window());
  Region r = Region(rect);
  state_.damaged_region.Union(r);
}

}  // namespace naive
