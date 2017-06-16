#include "compositor/surface.h"

#include <algorithm>

#include "wm/window.h"
#include "compositor/buffer.h"

namespace naive {

Surface::Surface(): window_(std::make_unique<wm::Window>()) {
  window_->SetSurface(this);
}

void Surface::Attach(Buffer* buffer) {
  LOG_ERROR << "Surface::Attach " << buffer << std::endl;
  buffer->SetOwningSurface(this);
  window_->SetGeometry(base::geometry::Rect(0, 0, buffer->width(),
                                            buffer->height()));
  pending_state_.buffer = buffer;
}

void Surface::SetFrameCallback(std::function<void()>* callback) {
  pending_state_.frame_callback = callback;
}

void Surface::Damage(const base::geometry::Rect& rect) {
  Region region(rect);
  pending_state_.damaged_region.Union(region);
}

void Surface::SetOpaqueRegion(const Region& region) {
  pending_state_.opaque_region = region;
}

void Surface::SetInputRegion(const Region &region) {
  pending_state_.input_region = region;
}

void Surface::Commit() {
  LOG_ERROR << "calling Surface::Commit on window " << window() << std::endl;
  state_ = pending_state_;
  has_commit_ = true;

  if (state_.frame_callback) {
    (*state_.frame_callback)();
    state_.frame_callback = nullptr;
    pending_state_.frame_callback = nullptr;
  }

  for (auto observer: observers_)
    observer->OnCommit();
}

}  // namespace naive

