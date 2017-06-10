#include "compositor/surface.h"

#include <algorithm>

#include "wm/window.h"
#include "compositor/buffer.h"

namespace naive {

Surface::Surface(): window_(std::make_unique<wm::Window>()) {
  window_->SetSurface(this);
}

void Surface::Attach(Buffer* buffer) {
  buffer->SetOwningSurface(this);
  pending_state_.buffer = buffer;
}

void Surface::Damage(const base::geometry::Rect& rect) {
  pending_state_.damaged_region.Union(Region(rect));
}

void Surface::SetOpaqueRegion(const Region& region) {
  pending_state_.opaque_region = region;
}

void Surface::SetInputRegion(const Region &region) {
  pending_state_.input_region = region;
}

void Surface::Commit() {
  state_ = pending_state_;
}

}  // namespace naive

