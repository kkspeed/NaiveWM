#include "wm/window.h"

#include <algorithm>

#include "base/logging.h"
#include "compositor/shell_surface.h"
#include "wm/window_manager.h"

namespace naive {
namespace wm {

Window::Window()
    : surface_(nullptr), shell_surface_(nullptr), parent_(nullptr) {
  TRACE("creating %p", this);
}

Window::~Window() {
  TRACE("%p", this);
  wm::WindowManager::Get()->RemoveWindow(this);
  if (parent_)
    parent_->RemoveChild(this);
  surface_->RemoveSurfaceObserver(this);
  // Window is destroyed with surface. no need to remove surface observer
}

void Window::AddChild(Window* child) {
  children_.push_back(child);
  child->set_parent(this);
}

void Window::RemoveChild(Window* child) {
  auto iter = std::find(children_.begin(), children_.end(), child);
  if (iter != children_.end())
    children_.erase(iter);
  child->set_parent(nullptr);
}

bool Window::HasChild(const Window* child) const {
  auto iter = std::find(children_.begin(), children_.end(), child);
  return iter != children_.end();
}

void Window::PlaceAbove(Window* window, Window* target) {
  if (!HasChild(window)) {
    LOG_ERROR << "window " << window << " is not a child of " << this;
    return;
  }

  if (!HasChild(target)) {
    LOG_ERROR << "window target " << target << " is not a child of " << this;
    return;
  }

  RemoveChild(window);
  auto iter = std::find(children_.begin(), children_.end(), target);
  children_.insert(++iter, window);
}

void Window::PlaceBelow(Window* window, Window* target) {
  if (!HasChild(window)) {
    LOG_ERROR << "window " << window << " is not a child of " << this;
    return;
  }

  if (!HasChild(target)) {
    LOG_ERROR << "window target " << target << " is not a child of " << this;
    return;
  }

  RemoveChild(window);
  auto iter = std::find(children_.begin(), children_.end(), target);
  children_.insert(iter, window);
}

void Window::Resize(int32_t width, int32_t height) {
  if (!shell_surface_) {
    LOG_ERROR << " only shell surface can be resized. " << std::endl;
    return;
  }
  pending_state_.geometry.width_ = width;
  pending_state_.geometry.height_ = height;
  state_.geometry.width_ = width;
  state_.geometry.height_ = height;
  shell_surface_->Configure(width, height);
}

void Window::OnCommit() {
  state_ = pending_state_;
  if (to_be_managed_ && !managed_) {
    to_be_managed_ = false;
    // Detach this surface from parent since it's going to be managed at top
    // level
    if (!parent_)
      wm::WindowManager::Get()->Manage(this);
    else
      WmSetSize(geometry().width(), geometry().height());
  }
}

void Window::set_fullscreen(bool fullscreen) {
  if (!shell_surface_) {
    LOG_ERROR << " only shell surface can call set_fullscreen. " << std::endl;
    return;
  }

  shell_surface_->Configure(pending_state_.geometry.width(),
                            pending_state_.geometry.height());
}

void Window::set_maximized(bool maximized) {
  if (!shell_surface_) {
    LOG_ERROR << " only shell surface can call set_maximized. " << std::endl;
    return;
  }

  shell_surface_->Configure(pending_state_.geometry.width(),
                            pending_state_.geometry.height());
}

void Window::WmSetSize(int32_t width, int32_t height) {
  if (!shell_surface_) {
    LOG_ERROR << " only shell surface can set window size." << std::endl;
    return;
  }

  shell_surface_->Configure(width, height);
}

void Window::LoseFocus() {
  if (!shell_surface_) {
    LOG_ERROR << " only shell surface can do focus." << std::endl;
    return;
  }
  if (focused_) {
    focused_ = false;
    // TODO: Not seem to need to configure?
  }
}

void Window::TakeFocus() {
  if (!shell_surface_) {
    LOG_ERROR << " only shell surface can do focus." << std::endl;
    return;
  }

  // TODO: Revisit focus!
  if (is_popup() || is_transient())
    return;

  if (!focused_) {
    focused_ = true;
    auto size = geometry();
    shell_surface_->Configure(size.width(), size.height());
  }
}

void Window::Raise() {
  auto* window_manager = wm::WindowManager::Get();
  window_manager->RaiseWindow(this);
}

void Window::Close() {
  if (!shell_surface_) {
    LOG_ERROR << " only shell surface can call close." << std::endl;
    return;
  }
  shell_surface_->Close();
}

}  // namespace wm
}  // namespace naive
