#include "wm/manage/manage_hook.h"

#include <linux/input.h>
#include <signal.h>
#include <memory>
#include <vector>

#include "base/image_codec.h"
#include "base/time.h"
#include "base/utils.h"
#include "compositor/compositor.h"
#include "ui/image_view.h"
#include "wayland/display_metrics.h"
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
constexpr int32_t kWorkspaceInsetY = 10;
}  // namespace

ManageHook::ManageHook() {
  for (uint32_t i = 0; i < 9; i++) {
    workspaces_.push_back(Workspace(i, kWorkspaceInsetY));
    current_workspace_ = 0;
  }
}

void ManageHook::PostWmInitialize() {
  auto* display_metrics = compositor::Compositor::Get()->GetDisplayMetrics();
  wallpaper_view_ = std::make_unique<ui::ImageView>(
      0, 0, display_metrics->width_pixels, display_metrics->height_pixels,
      kWallpaperPath);
  wm::WindowManager::Get()->set_wallpaper_window(wallpaper_view_->window());

  panel_ = std::make_unique<Panel>(0, 0, display_metrics->width_pixels, 20);
  wm::WindowManager::Get()->set_panel_window(panel_->window());
}

void ManageHook::WindowCreated(Window* window) {
  TRACE();
  if (window->parent()) {
    window->WmSetSize(window->geometry().width(), window->geometry().height());
    return;
  }
  auto* workspace = current_workspace();
  auto* mw =
      workspace->AddWindow(std::make_unique<ManageWindow>(window, primitives_));
  window_added_callback_.erase(std::remove_if(window_added_callback_.begin(),
                                              window_added_callback_.end(),
                                              [mw](auto& f) {
                                                bool res = f(mw);
                                                return res;
                                              }),
                               window_added_callback_.end());
  primitives_->FocusWindow(workspace->CurrentWindow()->window());
  workspace->ArrangeWindows(kWorkspaceInsetX, kWorkspaceInsetY,
                            width_ - kWorkspaceInsetX,
                            height_ - kWorkspaceInsetY);
}

void ManageHook::WindowDestroying(Window* window) {
  TRACE();
  if (scoped_move_window_)
    scoped_move_window_->OnWindowDestroying(window);
  if (scoped_resize_window_)
    scoped_resize_window_->OnWindowDestroying(window);
}

void ManageHook::WindowDestroyed(Window* window) {
  TRACE("window: %p", window);
  for (Workspace& workspace : workspaces_) {
    if (workspace.HasWindow(window)) {
      auto mw = workspace.PopWindow(window);
      // TODO: No need to set visibility anymore?
      mw->Show(false);
      window_removed_callback_.erase(
          std::remove_if(window_removed_callback_.begin(),
                         window_removed_callback_.end(),
                         [&mw](auto& f) { return f(mw.get()); }),
          window_removed_callback_.end());
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
      const char* args[] = {"lxtemrinal", "--no-remote", nullptr};
      base::LaunchProgram("lxterminal", (char**)args);
    }
    return true;
  }

#ifdef NO_XWAYLAND
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
#endif

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

  if (event->super_pressed() && event->keycode() == KEY_R) {
    if (event->pressed())
      return true;
    const char* args[] = {"kupfer", nullptr};
    pid_t pid = base::LaunchProgram("kupfer", (char**)args);
    window_added_callback_.push_back([pid](ManageWindow* mw) {
      if (mw->window()->GetPid() == pid) {
        mw->set_floating(true);
        mw->MoveResize(0, 10, 600, 300);
        mw->window()->enable_border(false);
        return true;
      }
      return false;
    });

    window_removed_callback_.push_back([pid](ManageWindow* mw) {
      if (mw->window()->GetPid() == pid) {
        kill(pid, SIGTERM);
        return true;
      }
      return false;
    });

    return true;
  }

  if (event->super_pressed() && event->keycode() == KEY_J) {
    if (event->pressed())
      return true;
    auto* next = current_workspace()->NextVisibleWindow();
    primitives_->FocusWindow(next ? next->window() : nullptr);
    return true;
  }

  if (event->super_pressed() && event->keycode() == KEY_K) {
    if (event->pressed())
      return true;
    auto* prev = current_workspace()->PrevVisibleWindow();
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
            &base::EncodePngToFile,
            base::Time::GetTime("/tmp/screenshot-%Y-%m-%d-%H_%M_%S.png"),
            std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3)));
    TRACE("Screenshot saved...");
    return true;
  }

  if (event->super_pressed() && event->keycode() == KEY_D) {
    if (event->pressed())
      return true;
    if (popup_terminal_pid_ == 0) {
      const char* args[] = {"lxterminal", "--no-remote", nullptr};
      pid_t pid = base::LaunchProgram("lxterminal", (char**)args);
      popup_terminal_pid_ = pid;
      window_added_callback_.push_back([this](ManageWindow* mw) {
        if (this->popup_terminal_pid_ &&
            mw->window()->GetPid() == this->popup_terminal_pid_) {
          mw->MoveResize(0, 10, 1280, 450);
          mw->set_floating(true);
          mw->SetShowPredicate([](bool show, ManageWindowShowReason reason) {
            return show && reason == ManageWindowShowReason::SHOW_TOGGLE;
          });
          return true;
        }
        return false;
      });

      window_removed_callback_.push_back([this](ManageWindow* mw) {
        if (mw->window()->GetPid() == this->popup_terminal_pid_) {
          this->popup_terminal_pid_ = 0;
          return true;
        }
        return false;
      });

      return true;
    }
    ManageWindow* window =
        current_workspace()->FindWindowByPid(popup_terminal_pid_);
    ManageWindow* current = current_workspace()->CurrentWindow();
    bool visible = window->window()->is_visible();
    window->Show(!visible);
    if (!visible) {
      current_workspace()->SetCurrentWindow(window->window());
      primitives_->RaiseWindow(window->window());
      primitives_->FocusWindow(window->window());
    } else if (current == window) {
      auto* next = current_workspace()->NextWindow();
      primitives_->FocusWindow(next->window());
    }

    return true;
  }

  return false;
}

bool ManageHook::OnMouseEvent(MouseEvent* event) {
  if (event->super_pressed()) {
    // TODO: collpase these into single function
    if (event->type() == wm::MouseEventType::MouseButtonDown &&
        event->get_button() == BTN_LEFT) {
      if (!event->window() || !event->window()->is_popup())
        return true;
      scoped_move_window_.reset(
          new wm::ScopedMoveWindow(event->window(), event->x(), event->y()));
      return true;
    }

    if (event->type() == wm::MouseEventType::MouseButtonUp &&
        event->get_button() == BTN_LEFT) {
      scoped_move_window_.reset();
      return true;
    }

    if (event->type() == wm::MouseEventType::MouseMotion) {
      if (scoped_move_window_) {
        scoped_move_window_->OnMouseMove(event->x(), event->y());
        return true;
      }
    }

    if (event->type() == wm::MouseEventType::MouseButtonDown &&
        event->get_button() == BTN_RIGHT) {
      if (!event->window() ||
          (!event->window()->is_popup() && event->window()->is_top_level()))
        return true;
      scoped_resize_window_.reset(
          new wm::ScopedResizeWindow(event->window(), event->x(), event->y()));
      return true;
    }

    if (event->type() == wm::MouseEventType::MouseButtonUp &&
        event->get_button() == BTN_RIGHT) {
      scoped_resize_window_.reset();
      return true;
    }

    if (event->type() == wm::MouseEventType::MouseMotion) {
      if (scoped_resize_window_) {
        scoped_resize_window_->OnMouseMove(event->x(), event->y());
        return true;
      }
    }
    return true;
  }

  auto* current_manage_window = current_workspace()->CurrentWindow();
  if (event->window() && event->type() == MouseEventType::MouseButtonDown) {
    auto* top_level = event->window()->top_level();
    if (current_workspace()->HasWindow(top_level))
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
  if (current_workspace_ == tag)
    return;
  previous_tag_ = current_workspace_;
  if (popup_terminal_pid_) {
    ManageWindow* window =
        current_workspace()->FindWindowByPid(popup_terminal_pid_);
    MoveWindowToTag(window->window(), tag);
  }
  current_workspace()->Show(false);
  current_workspace_ = tag;
  current_workspace()->Show(true);
  auto* manage_window = current_workspace()->CurrentWindow();
  auto* start_window = manage_window;
  while (manage_window && !manage_window->window()->is_visible()) {
    manage_window = current_workspace()->NextWindow();
    if (manage_window == start_window) {
      manage_window = nullptr;
      break;
    }
  }

  primitives_->FocusWindow(manage_window ? manage_window->window() : nullptr);
  current_workspace()->ArrangeWindows(kWorkspaceInsetX, kWorkspaceInsetY,
                                      width_ - kWorkspaceInsetX,
                                      height_ - kWorkspaceInsetY);
  std::vector<int32_t> window_count;
  for (auto& workspace : workspaces_)
    window_count.push_back(workspace.window_count());
  panel_->OnWorkspaceChanged(tag, window_count);
}

void ManageHook::MoveWindowToTag(Window* window, size_t tag) {
  assert(window);
  // TODO: Rename this to move current window?
  auto manage_window = current_workspace()->PopWindow(window);
  assert(manage_window);
  manage_window->Show(false, ManageWindowShowReason::SHOW_WORKSPACE_CHANGE);
  workspaces_[tag].AddWindow(std::move(manage_window));
  TRACE("arrange for workspace %d", current_workspace()->tag());
  auto* current = current_workspace()->CurrentWindow();
  primitives_->FocusWindow(current ? current->window() : nullptr);
  current_workspace()->ArrangeWindows(kWorkspaceInsetX, kWorkspaceInsetY,
                                      width_ - kWorkspaceInsetX,
                                      height_ - kWorkspaceInsetY);
  // TODO: Merge this with L341, maybe passing workspace?
  std::vector<int32_t> window_count;
  for (auto& workspace : workspaces_)
    window_count.push_back(workspace.window_count());
  panel_->OnWorkspaceChanged(current_workspace_, window_count);
}

}  // namespace wm
}  // namespace naive
