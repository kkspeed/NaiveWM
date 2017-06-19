#include <vector>

#include "wm/manage_hook.h"
#include "wm/mouse_event.h"
#include "wm/window.h"
#include "wm/window_manager.h"

namespace naive {
namespace wm {

namespace {

void TileWindows(Window* window, std::vector<Window*> windows,
                 int32_t width, int32_t height) {
  if (windows.size() == 1) {
    window->WmSetPosition(0, 0);
    window->WmSetSize(width, height);
  }
  if (windows.size() == 2) {
    windows[0]->WmSetPosition(0, 0);
    windows[0]->WmSetSize(width / 2, height);
    windows[1]->WmSetPosition(width / 2, 0);
    windows[1]->WmSetSize(width / 2, height);
  }
}

}  // namespace

void ManageHook::WindowCreated(Window* window) {
  TRACE();
  primitives_->FocusWindow(window);
  TileWindows(window, primitives_->windows(), width_, height_);
}

void ManageHook::WindowDestroyed(Window* window) {
  TRACE();
  auto* current_focus = primitives_->focused_window();
  if (current_focus == window) {
    auto* next_focus = primitives_->NextWindow(window);
    primitives_->FocusWindow(next_focus);
  }
}

bool ManageHook::OnKey(Event* event) {
  return false;
}

bool ManageHook::OnMouseEvent(MouseEvent* event) {
  return false;
}

}  // namespace wm
}  // namespace naive
