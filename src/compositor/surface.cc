#include "compositor/surface.h"

#include "wayland/buffer.h"

namespace naive {
namespace wayland {

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

void Commit() {
  state_ = pending_state_;
}

}  // namespace wayland
}  // namespace naive

