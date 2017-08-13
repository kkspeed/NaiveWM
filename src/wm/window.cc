#include "wm/window.h"

#include <algorithm>

#include "base/logging.h"
#include "compositor/compositor.h"
#include "compositor/shell_surface.h"
#include "wm/window_impl/window_impl_cairo.h"
#include "wm/window_impl/window_impl_wayland.h"
#include "wm/window_manager.h"
#include "ui/widget.h"

namespace naive {
namespace wm {

Window::Window(Surface* surface)
    : surface_(surface),
      shell_surface_(nullptr),
      parent_(nullptr),
      widget_(nullptr),
      type_(WindowType::NORMAL),
      window_impl_(std::make_unique<WindowImplWayland>(surface)) {
  TRACE("creating window: %p, surface: %p", this, surface);
}

Window::Window(ui::Widget* widget)
    : surface_(nullptr),
      type_(WindowType::WIDGET),
      widget_(widget),
      window_impl_(std::make_unique<WindowImplCairo>(widget)) {}

Window::~Window() {
  TRACE("%p", this);
  compositor::Compositor::Get()->AddGlobalDamage(global_bound());
  wm::WindowManager::Get()->RemoveWindow(this);
  if (parent_) {
    TRACE("removing window %p from parent: %p", this, parent_);
    parent_->RemoveChild(this);
  }
  for (auto* child : children_)
    child->set_parent(nullptr);
}

void Window::SetShellSurface(ShellSurface* shell_surface) {
  auto* impl = static_cast<WindowImplWayland*>(window_impl_.get());
  impl->set_shell_surface(shell_surface);
  shell_surface_ = shell_surface;
}

void Window::set_type(WindowType type) {
  type_ = type;
}

WindowType Window::window_type() const {
  return type_;
}

Surface* Window::surface() {
  assert(type_ == WindowType::NORMAL);
  return surface_;
}

void Window::AddChild(Window* child) {
  TRACE("adding %p as child of %p", child, this);
  if (child->parent() == nullptr)
    WindowManager::Get()->RemoveWindow(child);
  auto iter = std::find(children_.begin(), children_.end(), child);
  if (iter == children_.end())
    children_.push_back(child);
  child->set_parent(this);
}

void Window::RemoveChild(Window* child) {
  TRACE("removing child %p from parent: %p", child, this);
  auto iter = std::find(children_.begin(), children_.end(), child);
  if (iter != children_.end())
    children_.erase(iter);
  child->set_parent(nullptr);

  iter = std::find(children_.begin(), children_.end(), child);
  assert(iter == children_.end());
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

void Window::GrabDone() {
  if (!is_popup_) {
    TRACE("not a popup window: %p", this);
    return;
  }

  window_impl_->SurfaceUngrab();
}

void Window::PushProperty(const base::geometry::Rect& geometry,
                          const base::geometry::Rect& visible_region) {
  geometry_ = geometry;
  visible_region_ = visible_region;
}

void Window::PushProperty(bool is_position, int32_t v0, int32_t v1) {
  if (is_position) {
    geometry_.x_ = v0;
    geometry_.y_ = v1;
  } else {
    geometry_.width_ = v0;
    geometry_.height_ = v1;
  }
}

void Window::Resize(int32_t width, int32_t height) {
  if (!window_impl_->CanResize())
    return;

  geometry_.width_ = width;
  geometry_.height_ = height;
  window_impl_->Configure(width, height);
}

void Window::MaybeMakeTopLevel() {
  if (to_be_managed_ && !managed_) {
    // Detach this surface from parent since it's going to be managed at top
    // level
    if (!parent_) {
      has_border_ = true;
      wm::WindowManager::Get()->Manage(this);
    } else
      WmSetSize(geometry().width(), geometry().height());
    to_be_managed_ = false;
  }
}

void Window::set_fullscreen(bool fullscreen) {
  if (!shell_surface_) {
    LOG_ERROR << " only shell surface can call set_fullscreen. " << std::endl;
    return;
  }

  // TODO: changed from pending_state_ to state_... does it work?
  window_impl_->Configure(geometry_.width(), geometry_.height());
}

void Window::set_maximized(bool maximized) {
  if (!shell_surface_) {
    LOG_ERROR << " only shell surface can call set_maximized. " << std::endl;
    return;
  }

  // TODO: changed from pending_state_ to state_... does it work?
  window_impl_->Configure(geometry_.width(), geometry_.height());
}

void Window::WmSetSize(int32_t width, int32_t height) {
  if (!shell_surface_) {
    LOG_ERROR << " only shell surface can set window size." << std::endl;
    return;
  }

  window_impl_->Configure(width, height);
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
    window_impl_->Configure(size.width(), size.height());
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
  window_impl_->SurfaceClose();
}

void Window::set_visible(bool visible) {
  if (visible_ == visible)
    return;

  if (parent_ != nullptr) {
    TRACE("ERROR: shouldn't set visibility on non toplevel surface!");
    return;
  }

  TRACE("%p -> visible: %d", this, visible);
  if (visible)
    surface()->ForceDamage(geometry());
  compositor::Compositor::Get()->AddGlobalDamage(global_bound());
  visible_ = visible;
}

void Window::NotifyFrameCallback() {
  window_impl_->NotifyFrameRendered();
}

pid_t Window::GetPid() {
  if (type_ == WindowType::NORMAL) {
    auto* client = wl_resource_get_client(surface_->resource());
    pid_t pid;
    gid_t gid;
    uid_t uid;
    wl_client_get_credentials(client, &pid, &uid, &gid);
    return pid;
  }
  return 0;
}

}  // namespace wm
}  // namespace naive
