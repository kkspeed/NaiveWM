#ifndef WM_WINDOW_MANAGER_H_
#define WM_WINDOW_MANAGER_H_

#include <vector>

#include "base/geometry.h"
#include "event/event_hub.h"
#include "wm/window.h"

namespace naive {
namespace wm {

class WindowManager : event::EventObserver {
 public:
  WindowManager();
  static void InitializeWindowManager();
  static WindowManager* Get();
  void Manage(Window* window);
  bool PointerMoved();

  std::vector<Window*> windows() { return windows_; }
  base::geometry::FloatPoint mouse_position() { return mouse_position_; }
  base::geometry::FloatPoint last_mouse_position() { return last_mouse_position_; }

  // EventObserver overrides:
  void OnMouseMotion(float dx, float dy) override;

 private:
  static WindowManager* g_window_manager;
  std::vector<Window*> windows_;
  int screen_width_, screen_height_;

  base::geometry::FloatPoint mouse_position_;
  base::geometry::FloatPoint last_mouse_position_;
};

}  // namespace wm
}  // namespace naive

#endif  // WM_WINDOW_MANAGER_H_
