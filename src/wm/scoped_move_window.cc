#include "wm/scoped_move_window.h"

#include "wm/window.h"

namespace naive {
namespace wm {

ScopedMoveWindow::ScopedMoveWindow(Window* window, int32_t x, int32_t y)
    : window_(window),
      initial_x_(x),
      initial_y_(y),
      window_initial_x_(window->wm_x()),
      window_initial_y_(window->wm_y()) {
  TRACE("wx: %d, wy: %d, initial_x_: %d, initial_y_: %d", window_initial_x_,
        window_initial_y_, initial_x_, initial_y_);
  window_->AddWindowObserver(this);
}

ScopedMoveWindow::~ScopedMoveWindow() {
  if (window_)
    window_->RemoveWindowObserver(this);
}

void ScopedMoveWindow::OnMouseMove(int32_t x, int32_t y) {
  TRACE("x: %d, y: %d", x, y);
  if (!window_)
    return;
  int32_t delta_x = x - initial_x_;
  int32_t delta_y = y - initial_y_;
  window_->WmSetPosition(window_initial_x_ + delta_x,
                         window_initial_y_ + delta_y);
}

void ScopedMoveWindow::OnWindowDestroyed(Window* window) {
  if (window == window_)
    window_ = nullptr;
}

}  // namespace wm
}  // namespace naive
