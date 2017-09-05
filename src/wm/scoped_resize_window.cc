#include "wm/scoped_resize_window.h"

#include <cmath>

#include "wm/window.h"

namespace naive {
namespace wm {

ScopedResizeWindow::ScopedResizeWindow(Window* window,
                                       int32_t init_mouse_x,
                                       int32_t init_mouse_y)
    : window_(window),
      init_mouse_x_(init_mouse_x),
      init_mouse_y_(init_mouse_y),
      last_mouse_x_(init_mouse_x),
      last_mouse_y_(init_mouse_y),
      window_bounds_(window->geometry()) {
  window_->AddWindowObserver(this);
}

ScopedResizeWindow::~ScopedResizeWindow() {
  if (window_)
    window_->RemoveWindowObserver(this);
}

void ScopedResizeWindow::OnMouseMove(int32_t x, int32_t y) {
  if (!window_)
    return;
  int32_t motion_x = x - last_mouse_x_;
  int32_t motion_y = y - last_mouse_y_;
  if (abs(motion_x) <= 10 && abs(motion_y) <= 10)
    return;
  last_mouse_x_ = x;
  last_mouse_y_ = y;
  int32_t delta_x = x - init_mouse_x_;
  int32_t delta_y = y - init_mouse_y_;
  int32_t new_width = window_bounds_.width() + delta_x;
  int32_t new_height = window_bounds_.height() + delta_y;
  window_->WmSetSize(new_width, new_height);
}

void ScopedResizeWindow::OnWindowDestroyed(Window* window) {
  if (window == window_)
    window_ = nullptr;
}

}  // namespace wm
}  // namespace naive
