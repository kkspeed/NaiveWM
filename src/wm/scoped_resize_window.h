#ifndef WM_SCOPED_RESIZE_WINDOW_H_
#define WM_SCOPED_RESIZE_WINDOW_H_

#include <cstdint>

#include "base/geometry.h"
#include "wm/window.h"

namespace naive {
namespace wm {

class ScopedResizeWindow : WindowObserver {
 public:
  explicit ScopedResizeWindow(Window* window,
                              int32_t init_mouse_x,
                              int32_t init_mouse_y);
  ~ScopedResizeWindow();
  void OnMouseMove(int32_t x, int32_t y);

  // WindowObserver override
  void OnWindowDestroyed(Window* window) override;

 private:
  int32_t init_mouse_x_, init_mouse_y_;
  int32_t last_mouse_x_, last_mouse_y_;
  base::geometry::Rect window_bounds_;
  Window* window_;
};

}  // namespace wm
}  // namespace naive

#endif  // WM_SCOPED_RESIZE_WINDOW_H_
