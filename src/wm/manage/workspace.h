#ifndef WM_MANAGE_WORKSPACE_H_
#define WM_MANAGE_WORKSPACE_H_

#include <memory>
#include <vector>
#include <functional>

#include "base/macros.h"
#include "base/geometry.h"
#include "wm/window.h"
#include "wm/window_manager.h"

namespace naive {
namespace wm {

class ManageWindow {
 public:
  explicit ManageWindow(Window* window, WMPrimitives* primitives);

  bool is_floating() { return is_floating_; }
  Window* window() { return window_; }
  void Show(bool show) { window_->set_visible(show); }
  void MoveResize(int32_t x, int32_t y, int32_t width, int32_t height) {
    primitives_->MoveResizeWindow(
        window_, base::geometry::Rect(x, y, width, height));
  }

 private:
  Window* window_;
  WMPrimitives* primitives_;
  bool is_floating_;

  DISALLOW_COPY_AND_ASSIGN(ManageWindow);
};

class Workspace {
 public:
  explicit Workspace(uint32_t tag);

  void AddWindow(std::unique_ptr<ManageWindow> window);
  ManageWindow* CurrentWindow();
  std::unique_ptr<ManageWindow> PopWindow(Window* window);
  ManageWindow* NextWindow();
  ManageWindow* PrevWindow();
  void AddWindowToHead(std::unique_ptr<ManageWindow> window);
  void Show(bool show);
  void SetCurrentWindow(Window* window);
  void ArrangeWindows(int32_t width, int32_t height);
  bool HasWindow(Window* window);
  uint32_t tag() { return tag_; }

 private:
  uint32_t tag_ = 0;
  size_t current_window_ = 0;
  std::vector<std::unique_ptr<ManageWindow>> windows_;
};

}  // namespace wm
}  // namespace naive

#endif  // WM_MANAGE_WORKSPACE_H_
