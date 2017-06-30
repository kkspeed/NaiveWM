#include "wm/manage/manage_hook.h"

#include <linux/input.h>
#include <memory>
#include <vector>

#include "base/image_codec.h"
#include "base/utils.h"
#include "compositor/compositor.h"
#include "wm/keyboard_event.h"
#include "wm/mouse_event.h"
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
  if (window->parent()) {
    window->WmSetSize(window->geometry().width(), window->geometry().height());
    return;
  }
  auto* workspace = current_workspace();
  workspace->AddWindow(std::make_unique<ManageWindow>(window, primitives_));
  primitives_->FocusWindow(workspace->CurrentWindow()->window());
  workspace->ArrangeWindows(width_, height_);
}

void ManageHook::WindowDestroying(Window* window) {
  TRACE();
}

void ManageHook::WindowDestroyed(Window* window) {
  TRACE("window: %p", window);
  for (Workspace &workspace : workspaces_) {
    if (workspace.HasWindow(window)) {
      (workspace.PopWindow(window))->Show(false);
      if (workspace.tag() == current_workspace_) {
        ManageWindow* next_window = workspace.CurrentWindow();
        primitives_->FocusWindow(next_window ? next_window->window() : nullptr);
        // TODO: maybe pass this to workspace and coords for multiscreen?
        workspace.ArrangeWindows(width_, height_);
      }
      break;
    }
  }
}

bool ManageHook::OnKey(KeyboardEvent* event) {
  if (event->super_pressed() && event->keycode() == KEY_T) {
    if (!event->pressed()) {
      const char* args[] = {"gnome-terminal", nullptr};
      base::LaunchProgram("gnome-terminal", (char**) args);
    }
    return true;
  }
  if (!event->pressed() && event->super_pressed() && event->shift_pressed() &&
      event->keycode() == KEY_Q) {
    exit(0);
  }

  if (event->super_pressed() && event->keycode() >= KEY_1 &&
      event->keycode() <= KEY_9) {
    if (event->pressed())
      return true;
    size_t tag = event->keycode() - KEY_1;
    if (event->shift_pressed()) {
      ManageWindow* current = current_workspace()->CurrentWindow();
      if (current)
        MoveWindowToTag(current->window(), tag);
      current = current_workspace()->CurrentWindow();
      primitives_->FocusWindow(current ? current->window() : nullptr);
      return true;
    }
    SelectTag(tag);
    return true;
  }

  if (event->super_pressed() && event->keycode() == KEY_J) {
    if (event->pressed())
      return true;
    auto* next = current_workspace()->NextWindow();
    primitives_->FocusWindow(next ? next->window() : nullptr);
    return true;
  }

  if (event->super_pressed() && event->keycode() == KEY_K) {
    if (event->pressed())
      return true;
    auto* prev = current_workspace()->PrevWindow();
    primitives_->FocusWindow(prev ? prev->window() : nullptr);
    return true;
  }

  if (event->super_pressed() && event->keycode() == KEY_C) {
    if (event->pressed())
      return true;
    const char* args[] = {"qutebrowser", "--qt-arg", "platform", "wayland",
                          nullptr};
    base::LaunchProgram("qutebrowser", (char**) args);
    return true;
  }

  if (event->super_pressed() && event->keycode() == KEY_F4) {
    if (event->pressed())
      return true;
    auto* current = current_workspace()->CurrentWindow();
    if (current)
      current->window()->Close();
    return true;
  }

  if (event->super_pressed() && event->keycode() == KEY_ENTER) {
    if (event->pressed())
      return true;
    auto* current = current_workspace()->CurrentWindow();
    if (current) {
      auto poped = current_workspace()->PopWindow(current->window());
      current_workspace()->AddWindowToHead(std::move(poped));
      current_workspace()->SetCurrentWindow(current->window());
      current_workspace()->ArrangeWindows(width_, height_);
    }
    return true;
  }

  if (event->super_pressed() && event->keycode() == KEY_P) {
    if (event->pressed())
      return true;
    TRACE("Saving screenshot...");
    compositor::Compositor::Get()
        ->CopyScreen(std::make_unique<compositor::CopyRequest>(
            std::bind(&base::EncodePngToFile, "/tmp/output.png",
                      std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3)));
    TRACE("Screenshot saved...");
    return true;
  }

  return false;
}

bool ManageHook::OnMouseEvent(MouseEvent* event) {
  auto* current_manage_window = current_workspace()->CurrentWindow();
  if (event->window() && event->type() == MouseEventType::MouseButtonDown) {
    auto* top_level = event->window()->top_level();
    primitives_->FocusWindow(event->window());
    if (current_workspace()->HasWindow(top_level) &&
        (!current_manage_window ||
            current_manage_window->window() != top_level)) {
      current_workspace()->SetCurrentWindow(top_level);
      return true;
    }
  }
  return false;
}

void ManageHook::SelectTag(size_t tag) {
  current_workspace()->Show(false);
  current_workspace_ = tag;
  current_workspace()->Show(true);
  auto* manage_window = current_workspace()->CurrentWindow();
  primitives_->FocusWindow(manage_window ? manage_window->window() : nullptr);
  current_workspace()->ArrangeWindows(width_, height_);
}

void ManageHook::MoveWindowToTag(Window* window, size_t tag) {
  assert(window);
  // TODO: Rename this to move current window?
  auto manage_window = current_workspace()->PopWindow(window);
  assert(manage_window);
  manage_window->Show(false);
  workspaces_[tag].AddWindow(std::move(manage_window));
  TRACE("arrange for workspace %d", current_workspace()->tag());
  auto* current = current_workspace()->CurrentWindow();
  primitives_->FocusWindow(current ? current->window() : nullptr);
  current_workspace()->ArrangeWindows(width_, height_);
}

}  // namespace wm
}  // namespace naive
