#ifndef WM_MANAGE_MANAGE_HOOK_H_
#define WM_MANAGE_MANAGE_HOOK_H_

#include <cstdint>
#include <vector>

#include "wm/manage/workspace.h"
#include "wm/window_manager.h"

namespace naive {
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
  bool OnMouseEvent(MouseEvent* event) override;
  bool OnKey(KeyboardEvent* event) override;

  void set_wm_primitives(WMPrimitives* primitives) override {
    primitives_ = primitives;
  }

  Workspace* current_workspace() { return &workspaces_[current_workspace_]; }

  void SelectTag(size_t tag);
  void MoveWindowToTag(Window* window, size_t tag);

 private:
  std::vector<Workspace> workspaces_;
  size_t current_workspace_;

  int32_t width_ = 1366, height_ = 768;
  WMPrimitives* primitives_ = nullptr;
};

}  // wm
}  // naive

#endif  // WM_MANAGE_MANAGE_HOOK_H_
