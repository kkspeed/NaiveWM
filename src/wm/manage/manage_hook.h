#ifndef WM_MANAGE_MANAGE_HOOK_H_
#define WM_MANAGE_MANAGE_HOOK_H_

#include <cstdint>
#include <functional>
#include <vector>

#include "ui/image_view.h"
#include "wm/manage/panel.h"
#include "wm/manage/workspace.h"
#include "wm/window_manager.h"
#include "wm/scoped_move_window.h"
#include "wm/scoped_resize_window.h"

namespace naive {

namespace wayland {
class DisplayMetrics;
}  // namespace wayland

namespace wm {

class Window;
class WMPrimitives;
class Event;
class MouseEvent;

class ManageHook : public WmEventObserver {
 public:
  explicit ManageHook();

  void WindowCreated(Window* window) override;
  void WindowDestroying(Window* window) override;
  void WindowDestroyed(Window* window) override;
  void PostWmInitialize() override;
  bool OnMouseEvent(MouseEvent* event) override;
  bool OnKey(KeyboardEvent* event) override;

  void PostSetupPolicy();

  void set_wm_primitives(WMPrimitives* primitives) override {
    primitives_ = primitives;
  }

  void set_workspace_dimension(int32_t width, int32_t height) override {
    width_ = width;
    height_ = height;
  }

  Workspace* current_workspace() { return &workspaces_[current_workspace_]; }

  void SelectTag(size_t tag);
  void MoveWindowToTag(Window* window, size_t tag);

 private:
  std::vector<Workspace> workspaces_;
  size_t current_workspace_;

  int32_t width_, height_;
  WMPrimitives* primitives_ = nullptr;
  std::unique_ptr<ui::Widget> wallpaper_view_;
  std::unique_ptr<Panel> panel_;

  using ManageWindowPolicy = std::function<bool(ManageWindow* mw)>;
  std::vector<ManageWindowPolicy> window_added_callback_;
  std::vector<ManageWindowPolicy> window_removed_callback_;

  int32_t previous_tag_{0};
  pid_t popup_terminal_pid_{0};

  std::unique_ptr<wm::ScopedMoveWindow> scoped_move_window_;
  std::unique_ptr<wm::ScopedResizeWindow> scoped_resize_window_;

  wayland::DisplayMetrics* display_metrics_;
};

}  // wm
}  // naive

#endif  // WM_MANAGE_MANAGE_HOOK_H_
