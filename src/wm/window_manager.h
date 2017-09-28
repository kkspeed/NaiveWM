#ifndef WM_WINDOW_MANAGER_H_
#define WM_WINDOW_MANAGER_H_

#include <memory>
#include <vector>

#include "base/geometry.h"
#include "base/macros.h"
#include "event/event_hub.h"
#include "wm/mouse_event.h"
#include "wm/window.h"

namespace naive {
namespace wayland {

struct DisplayMetrics;

}  // namespace wayland

namespace wm {

class MouseEvent;
class KeyboardEvent;

class MouseObserver {
 public:
  virtual void OnMouseEvent(MouseEvent* event) = 0;
};

class KeyboardObserver {
 public:
  virtual void OnFocus(Window* window) = 0;
  virtual void OnKey(KeyboardEvent* event) = 0;
};

// A class for window manipulation primitives.
class WMPrimitives {
 public:
  virtual Window* focused_window() = 0;
  virtual std::vector<Window*> windows() = 0;
  virtual Window* NextWindow(Window* window) = 0;
  virtual Window* PreviousWindow(Window* window) = 0;
  virtual void FocusWindow(Window* window) = 0;
  virtual void MoveResizeWindow(Window* window,
                                base::geometry::Rect resize) = 0;
  virtual void RaiseWindow(Window* window) = 0;
};

class WmEventObserver {
 public:
  virtual void set_wm_primitives(WMPrimitives* primitives) = 0;
  virtual void set_workspace_dimension(int32_t width, int32_t height) = 0;
  virtual void PostWmInitialize() = 0;
  virtual bool OnMouseEvent(MouseEvent* event) = 0;
  virtual bool OnKey(KeyboardEvent* event) = 0;
  virtual void WindowCreated(Window* window) = 0;
  virtual void WindowDestroying(Window* window) = 0;
  virtual void WindowDestroyed(Window* window) = 0;
};

// Representation of a surface, with coordinates relative to global (0, 0)
class Layer {
 public:
  explicit Layer(Window* window, int32_t x, int32_t y)
      : window_(window), x_(x), y_(y) {}

  int32_t x() { return x_; }
  int32_t y() { return y_; }

  Window* window() { return window_; }
  base::geometry::Rect geometry() {
    base::geometry::Rect rect(window_->geometry());
    rect.x_ = x_;
    rect.y_ = y_;
    return rect;
  }

 private:
  int32_t x_, y_;
  Window* window_;

  DISALLOW_COPY_AND_ASSIGN(Layer);
};

class WindowManager : public event::EventObserver, public WMPrimitives {
 public:
  WindowManager(WmEventObserver* wm_event_observer, event::EventHub* event_hub);
  static void InitializeWindowManager(WmEventObserver* wm_event_observer,
                                      event::EventHub* event_hub);
  static WindowManager* Get();
  void Manage(Window* window);
  void RemoveWindow(Window* window);
  void AddMouseObserver(MouseObserver* observer);
  void RemoveMouseObserver(MouseObserver* observer);
  void AddKeyboardObserver(KeyboardObserver* observer);
  void RemoveKeyboardObserver(KeyboardObserver* observer);
  void GlobalGrabWindow(Window* window);

  bool pointer_moved();
  base::geometry::FloatPoint mouse_position() { return mouse_position_; }
  base::geometry::FloatPoint last_mouse_position() {
    return last_mouse_position_;
  }

  // EventObserver overrides:
  void OnMouseButton(uint32_t button,
                     bool pressed,
                     uint32_t modifiers,
                     event::Leds locks) override;
  void OnMouseMotion(float dx,
                     float dy,
                     uint32_t modifiers,
                     event::Leds locks) override;
  void OnKey(uint32_t keycode,
             uint32_t modifiers,
             bool key_down,
             event::Leds locks) override;
  void OnMouseScroll(float x_scroll,
                     float y_scroll,
                     uint32_t modifiers,
                     event::Leds locks) override;

  void DispatchMouseEvent(std::unique_ptr<MouseEvent> event);
  Window* FindMouseEventTarget();

  std::vector<std::unique_ptr<Layer>> WindowsInGlobalCoordinates();

  // WMPrimitives overrides:
  std::vector<Window*> windows() override { return windows_; }
  Window* focused_window() override { return focused_window_; }
  Window* NextWindow(Window* window) override;
  Window* PreviousWindow(Window* window) override;
  void MoveResizeWindow(Window* window, base::geometry::Rect resize) override;
  void FocusWindow(Window* window) override;
  void RaiseWindow(Window* window) override;

  void set_mouse_pointer(Window* pointer) {
    mouse_pointer_ = pointer;
    pointer_updated_ = true;
  }
  Window* mouse_pointer() { return mouse_pointer_; }
  bool pointer_updated() { return pointer_updated_; }
  void clear_pointer_update() { pointer_updated_ = false; }

  void set_wallpaper_window(Window* wallpaper) {
    wallpaper_window_ = wallpaper;
  }
  Window* wallpaper_window() { return wallpaper_window_; }

  void set_panel_window(Window* panel) { panel_window_ = panel; }
  Window* panel_window() { return panel_window_; }

  using WindowPolicyAction = std::function<bool(Window*)>;

  void AddWindowPolicyAction(WindowPolicyAction action, bool once = false) {
    if (once) {
      policy_actions_once_.push_back(action);
      return;
    }
    policy_actions_.push_back(action);
  }

  Window* FindWindowByPid(pid_t pid) {
    auto iter = std::find_if(windows_.begin(), windows_.end(),
                             [pid](Window* w) { return w->GetPid() == pid; });
    return (iter == windows_.end()) ? nullptr : *iter;
  }

  void SetInputPanelTopLevel(Window* window) {
    input_panel_top_level_ = window;
  }
  void SetInputPanelOverlay(Window* window) { input_panel_overlay_ = window; }

  Window* input_panel_top_level() { return input_panel_top_level_; }
  Window* input_panel_overlay() { return input_panel_overlay_; }

  void DumpWindowHierarchy();

 private:
  static WindowManager* g_window_manager;
  std::vector<Window*> windows_;
  std::vector<KeyboardObserver*> keyboard_observers_;
  std::vector<MouseObserver*> mouse_observers_;
  int screen_width_, screen_height_;

  base::geometry::FloatPoint mouse_position_;
  base::geometry::FloatPoint last_mouse_position_;

  Window* focused_window_ = nullptr;
  Window* global_grab_window_ = nullptr;
  WmEventObserver* wm_event_observer_ = nullptr;
  wayland::DisplayMetrics* display_metrics_;

  Window* mouse_pointer_ = nullptr;
  bool pointer_updated_ = false;
  Window* wallpaper_window_ = nullptr;
  Window* panel_window_ = nullptr;
  Window* input_panel_top_level_ = nullptr;
  Window* input_panel_overlay_ = nullptr;
  std::vector<WindowPolicyAction> policy_actions_;
  std::vector<WindowPolicyAction> policy_actions_once_;
};

}  // namespace wm
}  // namespace naive

#endif  // WM_WINDOW_MANAGER_H_
