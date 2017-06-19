#ifndef WM_WINDOW_MANAGER_H_
#define WM_WINDOW_MANAGER_H_

#include <memory>
#include <vector>

#include "base/geometry.h"
#include "event/event_hub.h"
#include "wm/mouse_event.h"
#include "wm/window.h"

namespace naive {
namespace wm {

class MouseEvent;

class MouseObserver {
 public:
  virtual void OnMouseEvent(MouseEvent* event) = 0;
};

// A class for window manipulation primitives.
class WMPrimirtives {
 public:
  virtual Window* focused_window() = 0;
  virtual Window* NextWindow(Window* window) = 0;
  virtual Window* PreviousWindow(Window* window) = 0;
  virtual void FocusWindow(Window* window) = 0;
  virtual void MoveResizeWindow(Window* window,
                                base::geometry::Rect resize) = 0;
};

class WindowManager : public event::EventObserver,
                      public WMPrimirtives {
 public:
  WindowManager();
  static void InitializeWindowManager();
  static WindowManager* Get();
  void Manage(Window* window);
  void RemoveWindow(Window* window);
  void AddMouseObserver(MouseObserver* observer);
  void RemoveMouseObserver(MouseObserver* observer);

  std::vector<Window*> windows() { return windows_; }
  bool pointer_moved();
  base::geometry::FloatPoint mouse_position() { return mouse_position_; }
  base::geometry::FloatPoint last_mouse_position() { return last_mouse_position_; }

  // EventObserver overrides:
  void OnMouseButton(uint32_t button, bool pressed) override;
  void OnMouseMotion(float dx, float dy) override;

  void DispatchMouseEvent(std::unique_ptr<MouseEvent> event);
  Window* FindMouseEventTarget();

  // WMPrimitives overrides:
  Window* focused_window() override { return focused_window_; }
  Window* NextWindow(Window* window) override;
  Window* PreviousWindow(Window* window) override;
  void MoveResizeWindow(Window* window,
                        base::geometry::Rect resize) override;
  void FocusWindow(Window* window) override;

 private:
  static WindowManager* g_window_manager;
  std::vector<Window*> windows_;
  std::vector<MouseObserver*> mouse_observers_;
  int screen_width_, screen_height_;

  base::geometry::FloatPoint mouse_position_;
  base::geometry::FloatPoint last_mouse_position_;

  Window* focused_window_ = nullptr;
};

}  // namespace wm
}  // namespace naive

#endif  // WM_WINDOW_MANAGER_H_
