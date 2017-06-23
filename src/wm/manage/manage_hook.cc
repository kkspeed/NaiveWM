#include "wm/manage/manage_hook.h"

#include <linux/input.h>
#include <memory>
#include <vector>

#include "base/utils.h"
#include "wm/mouse_event.h"
#include "wm/keyboard_event.h"
#include "wm/window.h"
#include "wm/window_manager.h"

namespace naive {
namespace wm {

ManageHook::ManageHook() {
  for (uint32_t i = 0; i < 9; i++) {
    workspaces_.push_back(Workspace(i));
    current_workspace_ = 0;
  }
}

void ManageHook::WindowCreated(Window* window) {
  TRACE();
  auto* workspace = current_workspace();
  workspace->AddWindow(std::make_unique<ManageWindow>(window, primitives_));
  primitives_->FocusWindow(workspace->CurrentWindow()->window());
  workspace->ArrangeWindows();
}

void ManageHook::WindowDestroying(Window* window) {
  TRACE();
  for (Workspace& workspace : workspaces_) {
    if (workspace.HasWindow(window)) {
      (workspace.PopWindow(window))->Show(false);
      if (workspace.tag() == current_workspace_) {
        ManageWindow* next_window = workspace.CurrentWindow();
        primitives_->FocusWindow(next_window ? next_window->window() : nullptr);
        workspace.ArrangeWindows();
      }
      break;
    }
  }
}

void ManageHook::WindowDestroyed(Window* window) {
  TRACE();
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

  if (!event->pressed() && event->super_pressed() &&
      event->keycode() >= KEY_1 && event->keycode() <= KEY_9) {
    size_t tag = event->keycode() - KEY_1;
    if (event->shift_pressed()) {
      ManageWindow* current = current_workspace()->CurrentWindow();
      if (current)
        MoveWindowToTag(current->window(), tag);
      return true;
    }
    SelectTag(tag);
    return true;
  }

  // TODO: Close
  return false;
}

bool ManageHook::OnMouseEvent(MouseEvent* event) {
  auto* current_manage_window = current_workspace()->CurrentWindow();
  if (event->window() && event->type() == MouseEventType::MouseButtonDown) {
    auto* top_level = event->window()->top_level();
    if (current_workspace()->HasWindow(top_level) && (!current_manage_window ||
             current_manage_window->window() != top_level)) {
      current_workspace()->SetCurrentWindow(top_level);
      primitives_->FocusWindow(top_level);
      return true;
    }
  }
  return false;
}

void ManageHook::SelectTag(size_t tag) {
  current_workspace()->Show(false);
  current_workspace_ = tag;
  current_workspace()->Show(true);
  current_workspace()->ArrangeWindows();
  auto* manage_window = current_workspace()->CurrentWindow();
  primitives_->FocusWindow(manage_window ? manage_window->window() : nullptr);
}

void ManageHook::MoveWindowToTag(Window* window, size_t tag) {
  assert(window);
  // TODO: Rename this to move current window?
  auto manage_window = current_workspace()->PopWindow(window);
  assert(manage_window);
  manage_window->Show(false);
  workspaces_[tag].AddWindow(std::move(manage_window));
  current_workspace()->ArrangeWindows();
  auto* current_window = current_workspace()->CurrentWindow();
  primitives_->FocusWindow(current_window ? current_window->window() : nullptr);
}

}  // namespace wm
}  // namespace naive
