#ifndef WM_MANAGE_MANAGE_HOOK_H_
#define WM_MANAGE_MANAGE_HOOK_H_

#include <cstdint>
#include <functional>
#include <map>
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

  void RegisterAction(uint32_t modifier,
                      uint32_t keycode,
                      const std::function<void()>& action) {
    uint64_t key = ((static_cast<uint64_t>(modifier) << 32) | keycode);
    key_actions_[key] = action;
  }

  void WithCurrentWindow(const std::function<void(ManageWindow* mw)>& proc) {
    auto* current = current_workspace()->CurrentWindow();
    if (current)
      proc(current);
  }

  void ZoomWindow(ManageWindow* window);
  void MaximizeWindow(ManageWindow* mw);
  void TakeScreenshot();

 protected:
  class KeyFilter {
   public:
    enum Modifiers { Ctrl = 1, Shift = 1 << 1, Alt = 1 << 2, Super = 1 << 3 };

    KeyFilter(uint32_t modifiers, ManageHook* hook)
        : modifiers_{modifiers}, hook_{hook} {}

    KeyFilter operator+(KeyFilter& that) {
      KeyFilter result(modifiers_ | that.modifiers_, hook_);
      result.keycode_ = that.keycode_;
      return result;
    }

    KeyFilter operator+(uint32_t keycode) {
      keycode_ = keycode;
      return *this;
    }

    void Action(const std::function<void()>& action) {
      hook_->RegisterAction(modifiers_, keycode_, action);
    }

   private:
    uint32_t modifiers_ = 0;
    uint32_t keycode_ = 0;
    ManageHook* hook_;
  };

  virtual void AddWindowCallbacks();
  virtual void RegisterKeys();
  KeyFilter ctrl_, shift_, alt_, super_;

 private:
  uint64_t GetKey(KeyboardEvent* event);

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

  std::map<uint64_t, std::function<void()>> key_actions_;

  wayland::DisplayMetrics* display_metrics_;
};

}  // wm
}  // naive

#endif  // WM_MANAGE_MANAGE_HOOK_H_
