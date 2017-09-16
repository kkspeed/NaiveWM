#include "wm/manage/manage_hook.h"

#include <linux/input.h>
#include <signal.h>
#include <memory>
#include <vector>

#include "base/image_codec.h"
#include "base/time.h"
#include "base/utils.h"
#include "config.h"
#include "compositor/compositor.h"
#include "ui/image_view.h"
#include "ui/text_view.h"
#include "wayland/display_metrics.h"
#include "wm/keyboard_event.h"
#include "wm/manage/panel.h"
#include "wm/mouse_event.h"
#include "wm/window.h"
#include "wm/window_manager.h"

namespace naive {
namespace wm {

ManageHook::ManageHook()
    : ctrl_(KeyFilter::Ctrl, this),
      shift_(KeyFilter::Shift, this),
      alt_(KeyFilter::Alt, this),
      super_(KeyFilter::Super, this) {
  for (uint32_t i = 0; i < 9; i++) {
    workspaces_.push_back(Workspace(i, config::kWorkspaceInsetY));
    current_workspace_ = 0;
  }

  RegisterKeys();
}

void ManageHook::PostWmInitialize() {
  display_metrics_ = compositor::Compositor::Get()->GetDisplayMetrics();
  if (strnlen(config::kWallpaperPath, 255) == 0) {
    auto text_view = std::make_unique<ui::TextView>(
        0, 0, display_metrics_->width_pixels, display_metrics_->height_pixels);
    text_view->SetBackgroundColor(0xFF808080);
    text_view->SetTextSize(80);
    text_view->SetText("Simple! Sometimes Naive!");
    wallpaper_view_ = std::move(text_view);
  } else {
    wallpaper_view_ = std::make_unique<ui::ImageView>(
        0, 0, display_metrics_->width_pixels, display_metrics_->height_pixels,
        config::kWallpaperPath);
  }
  wm::WindowManager::Get()->set_wallpaper_window(wallpaper_view_->window());

  panel_ = std::make_unique<Panel>(0, 0, display_metrics_->width_pixels, 20);
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
  workspace->ArrangeWindows(config::kWorkspaceInsetX, config::kWorkspaceInsetY,
                            width_ - config::kWorkspaceInsetX,
                            height_ - config::kWorkspaceInsetY);
}

void ManageHook::WindowDestroying(Window* window) {
  TRACE();
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
        workspace.ArrangeWindows(config::kWorkspaceInsetX,
                                 config::kWorkspaceInsetY,
                                 width_ - config::kWorkspaceInsetX,
                                 height_ - config::kWorkspaceInsetY);
      }
      break;
    }
  }
}

bool ManageHook::OnKey(KeyboardEvent* event) {
  uint64_t key = GetKey(event);
  if (key_actions_.find(key) != key_actions_.end()) {
    if (event->pressed())
      return true;
    key_actions_[key]();
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
          mw->MoveResize(0, 10, this->display_metrics_->width_dp,
                         this->display_metrics_->height_dp * 0.6f);
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
  current_workspace()->ArrangeWindows(
      config::kWorkspaceInsetX, config::kWorkspaceInsetY,
      width_ - config::kWorkspaceInsetX, height_ - config::kWorkspaceInsetY);
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
  current_workspace()->ArrangeWindows(
      config::kWorkspaceInsetX, config::kWorkspaceInsetY,
      width_ - config::kWorkspaceInsetX, height_ - config::kWorkspaceInsetY);
  // TODO: Merge this with L341, maybe passing workspace?
  std::vector<int32_t> window_count;
  for (auto& workspace : workspaces_)
    window_count.push_back(workspace.window_count());
  panel_->OnWorkspaceChanged(current_workspace_, window_count);
}

void ManageHook::RegisterKeys() {
  (super_ + KEY_T).Action([this]() {
    const char* args[] = {"lxtemrinal", "--no-remote", nullptr};
    base::LaunchProgram("lxterminal", (char**)args);
  });

  (super_ + KEY_C).Action([this]() {
    const char* args[] = {"qutebrowser", "--qt-arg", "platform", "wayland",
                          nullptr};
    base::LaunchProgram("qutebrowser", (char**)args);
  });

  (super_ + KEY_F4).Action([this]() {
    this->WithCurrentWindow([](auto* w) { w->window()->Close(); });
  });

  (super_ + shift_ + KEY_Q).Action([]() { exit(0); });

  (super_ + KEY_ENTER).Action([this]() {
    this->WithCurrentWindow([this](auto* w) { this->ZoomWindow(w); });
  });

  (super_ + KEY_M).Action([this]() {
    this->WithCurrentWindow([this](auto* mw) { this->MaximizeWindow(mw); });
  });

  (super_ + KEY_TAB).Action([this]() { this->SelectTag(this->previous_tag_); });

  (super_ + KEY_J).Action([this]() {
    auto* next = this->current_workspace()->NextVisibleWindow();
    this->primitives_->FocusWindow(next ? next->window() : nullptr);
  });

  (super_ + KEY_K).Action([this]() {
    auto* prev = this->current_workspace()->PrevVisibleWindow();
    this->primitives_->FocusWindow(prev ? prev->window() : nullptr);
  });

  (super_ + KEY_P).Action([this]() { this->TakeScreenshot(); });

  for (uint32_t k = KEY_1; k <= KEY_9; k++) {
    uint32_t tag = k - KEY_1;
    (super_ + k).Action([tag, this]() { this->SelectTag(tag); });
    (super_ + shift_ + k).Action([tag, this]() {
      ManageWindow* current = this->current_workspace()->CurrentWindow();
      if (current)
        this->MoveWindowToTag(current->window(), tag);
      current = this->current_workspace()->CurrentWindow();
      this->primitives_->FocusWindow(current ? current->window() : nullptr);
    });
  }
}

uint64_t ManageHook::GetKey(KeyboardEvent* event) {
  uint64_t result = 0;
  if (event->ctrl_pressed())
    result |= KeyFilter::Ctrl;
  if (event->alt_pressed())
    result |= KeyFilter::Alt;
  if (event->super_pressed())
    result |= KeyFilter::Super;
  if (event->shift_pressed())
    result |= KeyFilter::Shift;
  return (result << 32) | event->keycode();
}

void ManageHook::ZoomWindow(ManageWindow* window) {
  auto poped = current_workspace()->PopWindow(window->window());
  current_workspace()->AddWindowToHead(std::move(poped));
  current_workspace()->SetCurrentWindow(window->window());
  current_workspace()->ArrangeWindows(
      config::kWorkspaceInsetX, config::kWorkspaceInsetY,
      width_ - config::kWorkspaceInsetX, height_ - config::kWorkspaceInsetY);
}

void ManageHook::MaximizeWindow(ManageWindow* mw) {
  mw->set_maximized(!mw->is_maximized());
  current_workspace()->ArrangeWindows(
      config::kWorkspaceInsetX, config::kWorkspaceInsetY,
      width_ - config::kWorkspaceInsetX, height_ - config::kWorkspaceInsetY);
}

void ManageHook::TakeScreenshot() {
  TRACE("Saving screenshot...");
  compositor::Compositor::Get()->CopyScreen(
      std::make_unique<compositor::CopyRequest>(std::bind(
          &base::EncodePngToFile,
          base::Time::GetTime("/tmp/screenshot-%Y-%m-%d-%H_%M_%S.png"),
          std::placeholders::_1, std::placeholders::_2,
          std::placeholders::_3)));
  TRACE("Screenshot saved...");
}

}  // namespace wm
}  // namespace naive
