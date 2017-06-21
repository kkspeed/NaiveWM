#include "wm/manage_hook.h"

#include <linux/input.h>
#include <vector>

#include "base/utils.h"
#include "wm/mouse_event.h"
#include "wm/keyboard_event.h"
#include "wm/window.h"
#include "wm/window_manager.h"

namespace naive {
namespace wm {

namespace {

void TileWindows(Window* window,
                 std::vector<Window*> windows,
                 int32_t width,
                 int32_t height) {
  if (windows.size() == 1) {
    windows[0]->WmSetPosition(0, 0);
    windows[0]->WmSetSize(width, height);
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
  if (window->is_popup() || window->is_transient())
    return;
  primitives_->FocusWindow(window);
  TileWindows(window, primitives_->windows(), width_, height_);
}

void ManageHook::WindowDestroying(Window* window) {
  TRACE();
  auto* current_focus = primitives_->focused_window();
  if (current_focus == window) {
    auto* next_focus = primitives_->NextWindow(window);
    primitives_->FocusWindow(next_focus);
  }
}

void ManageHook::WindowDestroyed(Window* window) {
  TRACE();
  TileWindows(primitives_->focused_window(), primitives_->windows(), width_,
              height_);
}

bool ManageHook::OnKey(KeyboardEvent* event) {
  if (!event->pressed() && event->super_pressed()
      && event->keycode() == KEY_T) {
    base::LaunchProgram("lxterminal", {});
    return true;
  }
  if (!event->pressed() && event->super_pressed() && event->shift_pressed() &&
      event->keycode() == KEY_Q) {
    exit(0);
  }
  return false;
}

bool ManageHook::OnMouseEvent(MouseEvent* event) {
  if (event->type() == MouseEventType::MouseButtonDown && event->window() &&
      event->window() != primitives_->focused_window()) {
    primitives_->FocusWindow(event->window());
    // return true;
  }
  return false;
}

}  // namespace wm
}  // namespace naive
