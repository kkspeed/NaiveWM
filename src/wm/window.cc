#include "window.h"

#include <algorithm>

#include "base/logging.h"
#include "compositor/shell_surface.h"

namespace naive {
namespace wm {

Window::Window()
    : surface_(nullptr),
      shell_surface_(nullptr),
      parent_(nullptr),
      x_(0),
      y_(0),
      width_(0),
      height_(0) {}

void Window::AddChild(Window* child) {
  children_.push_back(child);
  child->SetParent(this);
}

void Window::RemoveChild(Window* child) {
  auto iter = std::find(children_.begin(), children_.end(), child);
  if (iter != children_.end())
    children_.erase(iter);
  child->SetParent(nullptr);
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

void Window::Resize(int32_t width, int32_t height)  {
  if (!shell_surface_) {
    LOG_ERROR << " only shell surface can be resized. " << std::endl;
    return;
  }
  width_ = width;
  height_ = height;
  shell_surface_->Configure(width, height);
}

}  // namespace wm
}  // namespace naive
