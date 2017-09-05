#ifndef WM_SCOPED_MOVE_WINDOW_H_
#define WM_SCOPED_MOVE_WINDOW_H_

#include <cstdint>

#include "wm/window.h"

namespace naive {
namespace wm {

class ScopedMoveWindow : WindowObserver {
 public:
  explicit ScopedMoveWindow(Window* window, int32_t x, int32_t y);
  ~ScopedMoveWindow();

  void OnMouseMove(int32_t x, int32_t y);

  // WindowObserver overrides.
  void OnWindowDestroyed(Window* window) override;

 private:
  Window* window_;
  int32_t initial_x_, initial_y_;
  int32_t window_initial_x_, window_initial_y_;
};

}  // namespace wm
}  // namespace naive

#endif  // WM_SCOPED_MOVE_WINDOW_H_
