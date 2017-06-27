#include "compositor/surface.h"

#include <algorithm>

#include "compositor/buffer.h"
#include "wm/window.h"

namespace naive {

Surface::Surface() : window_(std::make_unique<wm::Window>()) {
  window_->set_surface(this);
}

void Surface::Attach(Buffer* buffer) {
  LOG_ERROR << "Surface::Attach " << buffer << " to window " << window()
            << std::endl;
  if (buffer) {
    buffer->SetOwningSurface(this);
    window_->SetGeometry(base::geometry::Rect(
        window_->pending_geometry().x(), window_->pending_geometry().y(),
        buffer->width(), buffer->height()));
  }
  pending_state_.buffer = buffer;
}

void Surface::SetFrameCallback(std::function<void()>* callback) {
  pending_state_.frame_callback = callback;
}

void Surface::Damage(const base::geometry::Rect& rect) {
  Region region(rect);
  pending_state_.damaged_region.Union(region);
}

void Surface::SetOpaqueRegion(const Region region) {
  pending_state_.opaque_region = region;
}

void Surface::SetInputRegion(const Region region) {
  pending_state_.input_region = region;
}

void Surface::Commit() {
  LOG_ERROR << "calling Surface::Commit on window " << window()
            << " surface: " << this << std::endl;
  state_ = pending_state_;
  if (state_.buffer && state_.buffer->data()) {
    state_.buffer->CopyLocal();
    state_.buffer->Release();
  } else {
    LOG_ERROR << "window: " << window() << " does not have buffer" << std::endl;
  }
  has_commit_ = true;

  /*
  if (state_.frame_callback)
    (*state_.frame_callback)();
   */

  pending_state_.frame_callback = nullptr;
  pending_state_.buffer = nullptr;

  // TODO: Can't use iterator due to possible iterator invalidation...
  // needs revisit
  for (size_t i = 0; i < observers_.size(); i++) {
    auto* observer = observers_[i];
    LOG_ERROR << "notifying " << observer
              << " for commit on surface " << this << std::endl;
    observer->OnCommit();
  }
}

}  // namespace naive
