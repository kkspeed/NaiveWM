#include "wm/manage/manage_hook.h"

#include <linux/input.h>
#include <memory>
#include <vector>

#include "base/image_codec.h"
#include "base/utils.h"
#include "compositor/compositor.h"
#include "ui/image_view.h"
#include "wm/keyboard_event.h"
#include "wm/manage/panel.h"
#include "wm/mouse_event.h"
#include "wm/window.h"
#include "wm/window_manager.h"

namespace naive {
namespace wm {

namespace {
// Change this to your wallpaper's path.
constexpr char kWallpaperPath[] = "/home/bruce/Downloads/test_wallpaper.png";
constexpr int32_t kWorkspaceInsetX = 0;
constexpr int32_t kWorkspaceInsetY = 0;  // 20;
}  // namespace

ManageHook::ManageHook() {
  for (uint32_t i = 0; i < 9; i++) {
    workspaces_.push_back(Workspace(i, kWorkspaceInsetY));
    current_workspace_ = 0;
  }
}

void ManageHook::PostWmInitialize() {
  // wallpaper_view_ =
  //    std::make_unique<ui::ImageView>(0, 0, 2560, 1440, kWallpaperPath);
  // wm::WindowManager::Get()->set_wallpaper_window(wallpaper_view_->window());

  // panel_ = std::make_unique<Panel>(0, 0, 2560, 20);
  // wm::WindowManager::Get()->set_panel_window(panel_->window());
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
  workspace->ArrangeWindows(kWorkspaceInsetX, kWorkspaceInsetY,
                            width_ - kWorkspaceInsetX,
                            height_ - kWorkspaceInsetY);
}

void ManageHook::WindowDestroying(Window* window) {
  TRACE();
}

void ManageHook::WindowDestroyed(Window* window) {
  TRACE("window: %p", window);
  for (Workspace& workspace : workspaces_) {
    if (workspace.HasWindow(window)) {
      (workspace.PopWindow(window))->Show(false);
      if (workspace.tag() == current_workspace_) {
        ManageWindow* next_window = workspace.CurrentWindow();
        primitives_->FocusWindow(next_window ? next_window->window() : nullptr);
        // TODO: maybe pass this to workspace and coords for multiscreen?
        workspace.ArrangeWindows(kWorkspaceInsetX, kWorkspaceInsetY,
                                 width_ - kWorkspaceInsetX,
                                 height_ - kWorkspaceInsetY);
      }
      break;
    }
  }
}

bool ManageHook::OnKey(KeyboardEvent* event) {
  if (event->super_pressed() && event->keycode() == KEY_T) {
    if (!event->pressed()) {
      const char* args[] = {"gnome-terminal", nullptr};
      base::LaunchProgram("gnome-terminal", (char**)args);
    }
    return true;
  }

  // Launch X wayland and apply policy to for event scale to 1.
  if (event->super_pressed() && event->keycode() == KEY_X) {
    if (!event->pressed()) {
      const char* args[] = {"Xwayland", "+iglx", ":1", nullptr};
      pid_t pid = base::LaunchProgram("Xwayland", (char**)args);
      wm::WindowManager::Get()->AddWindowPolicyAction(
          [pid](wm::Window* window) {
            if (window->GetPid() == pid) {
              window->set_mouse_event_scale_override(1);
              window->enable_border(false);
              return true;
            }
            return false;
          },
          true);
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

  if (event->super_pressed() && event->keycode() == KEY_TAB) {
    if (event->pressed())
      return true;
    SelectTag(previous_tag_);
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
    base::LaunchProgram("qutebrowser", (char**)args);
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

  if (event->super_pressed() && event->keycode() == KEY_M) {
    if (event->pressed())
      return true;
    auto* current = current_workspace()->CurrentWindow();
    if (current) {
      current->set_maximized(!current->is_maximized());
      current_workspace()->ArrangeWindows(kWorkspaceInsetX, kWorkspaceInsetY,
                                          width_ - kWorkspaceInsetX,
                                          height_ - kWorkspaceInsetY);
    }
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
      current_workspace()->ArrangeWindows(kWorkspaceInsetX, kWorkspaceInsetY,
                                          width_ - kWorkspaceInsetX,
                                          height_ - kWorkspaceInsetY);
    }
    return true;
  }

  if (event->super_pressed() && event->keycode() == KEY_P) {
    if (event->pressed())
      return true;
    TRACE("Saving screenshot...");
    compositor::Compositor::Get()->CopyScreen(
        std::make_unique<compositor::CopyRequest>(std::bind(
            &base::EncodePngToFile, "/tmp/output.png", std::placeholders::_1,
            std::placeholders::_2, std::placeholders::_3)));
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

void ManageHook::PostSetupPolicy() {}

void ManageHook::SelectTag(size_t tag) {
  previous_tag_ = current_workspace_;
  current_workspace()->Show(false);
  current_workspace_ = tag;
  current_workspace()->Show(true);
  auto* manage_window = current_workspace()->CurrentWindow();
  primitives_->FocusWindow(manage_window ? manage_window->window() : nullptr);
  current_workspace()->ArrangeWindows(kWorkspaceInsetX, kWorkspaceInsetY,
                                      width_ - kWorkspaceInsetX,
                                      height_ - kWorkspaceInsetY);
  panel_->OnWorkspaceChanged(tag);
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
  current_workspace()->ArrangeWindows(kWorkspaceInsetX, kWorkspaceInsetY,
                                      width_ - kWorkspaceInsetX,
                                      height_ - kWorkspaceInsetY);
}

}  // namespace wm
}  // namespace naive
