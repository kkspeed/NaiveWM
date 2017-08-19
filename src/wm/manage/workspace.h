#ifndef WM_MANAGE_WORKSPACE_H_
#define WM_MANAGE_WORKSPACE_H_

#include <functional>
#include <memory>
#include <vector>

#include "base/geometry.h"
#include "base/macros.h"
#include "wm/window.h"
#include "wm/window_manager.h"

namespace naive {
namespace wm {

enum class ManageWindowShowReason { SHOW_WORKSPACE_CHANGE, SHOW_TOGGLE };

class ManageWindow {
 public:
  explicit ManageWindow(Window* window, WMPrimitives* primitives);

  void set_floating(bool floating) { is_floating_ = floating; }
  bool is_floating() { return is_floating_; }
  Window* window() { return window_; }
  void Show(bool show) { window_->set_visible(show); }
  void Show(bool show, ManageWindowShowReason reason) {
    if (show_predicate_(show, reason))
      window_->set_visible(show);
  }
  void MoveResize(int32_t x, int32_t y, int32_t width, int32_t height) {
    primitives_->MoveResizeWindow(window_,
                                  base::geometry::Rect(x, y, width, height));
  }

  void set_maximized(bool maximized) { maximized_ = maximized; }
  bool is_maximized() { return maximized_; }

  using ShowPredicate =
      std::function<bool(bool, ManageWindowShowReason reason)>;
  void SetShowPredicate(ShowPredicate predicate) {
    show_predicate_ = predicate;
  }

 private:
  Window* window_;
  WMPrimitives* primitives_;
  bool is_floating_;
  bool maximized_ = false;
  ShowPredicate show_predicate_{[](auto, auto) { return true; }};
  DISALLOW_COPY_AND_ASSIGN(ManageWindow);
};

class Workspace {
 public:
  explicit Workspace(uint32_t tag, uint32_t workspace_inset_y);

  ManageWindow* AddWindow(std::unique_ptr<ManageWindow> window);
  ManageWindow* CurrentWindow();
  std::unique_ptr<ManageWindow> PopWindow(Window* window);
  ManageWindow* NextWindow();
  ManageWindow* PrevWindow();
  ManageWindow* NextVisibleWindow();
  ManageWindow* PrevVisibleWindow();
  ManageWindow* WindowWithCondition(
      std::function<bool(ManageWindow* mw)> predicate,
      std::function<ManageWindow*()> advance_proc);
  void AddWindowToHead(std::unique_ptr<ManageWindow> window);
  void Show(bool show);
  void SetCurrentWindow(Window* window);
  void ArrangeWindows(int32_t x, int32_t y, int32_t width, int32_t height);
  bool HasWindow(Window* window);
  ManageWindow* FindWindowByPid(pid_t pid);
  uint32_t tag() { return tag_; }

 private:
  uint32_t tag_ = 0;
  uint32_t workspace_inset_y_ = 0;
  size_t current_window_ = 0;
  std::vector<std::unique_ptr<ManageWindow>> windows_;
};

}  // namespace wm
}  // namespace naive

#endif  // WM_MANAGE_WORKSPACE_H_
