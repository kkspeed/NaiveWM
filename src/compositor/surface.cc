#include "compositor/surface.h"

#include <algorithm>

#include "compositor/buffer.h"
#include "compositor/compositor.h"
#include "wayland/display_metrics.h"
#include "wm/window.h"

namespace naive {

Surface::Surface() : window_(std::make_unique<wm::Window>(this)) {}

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
  if (buffer) {
    buffer->SetOwningSurface(this);
    Damage(window_->geometry());
  }
  buffer_attached_dirty_ = true;
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
    observer->OnCommit(this);
  }

  // IF dirty buffer attach:
  if (state_.buffer && buffer_attached_dirty_) {
    wayland::DisplayMetrics* metrics =
        compositor::Compositor::Get()->GetDisplayMetrics();
    window_->PushProperty(false, state_.buffer->width() / metrics->scale,
                          state_.buffer->height() / metrics->scale);
    buffer_attached_dirty_ = false;
  }
}

void Surface::ForceDamage(base::geometry::Rect rect) {
  TRACE("force damage %s on %p", rect.ToString().c_str(), window());
  Region r = Region(rect);
  state_.damaged_region.Union(r);
}

}  // namespace naive
