#include "window_manager.h"

#include <algorithm>
#include <cassert>
#include "base/logging.h"

namespace naive {
namespace wm {

// static
WindowManager* WindowManager::g_window_manager = nullptr;

// static
void WindowManager::InitializeWindowManager() {
  g_window_manager = new WindowManager();
}

WindowManager* WindowManager::Get() {
  assert(g_window_manager != nullptr);
  return g_window_manager;
}

// TODO: use real dimension
WindowManager::WindowManager() :
  screen_width_(2560), screen_height_(1080),
  mouse_position_(1280.0f, 720.0f), last_mouse_position_(1280.0f, 720.0f) {
  event::EventHub::Get()->AddEventObserver(this);
}

void WindowManager::Manage(Window* window) {
  // TODO: window management policy
  windows_.push_back(window);
}

void WindowManager::RemoveWindow(Window* window) {
  auto iter = std::find(windows_.begin(), windows_.end(), window);
  if (iter != windows_.end()) {
    windows_.erase(iter);
    // TODO: compositor needs to be notified of this
  }
}

bool WindowManager::PointerMoved() {
  return last_mouse_position_.manhattan_distance(mouse_position_) >= 1.0;
}

void WindowManager::OnMouseMotion(float dx, float dy) {
  float new_x = mouse_position_.x() + dx;
  float new_y = mouse_position_.y() + dy;
  if (mouse_position_.x() + dx >= screen_width_)
    new_x = screen_width_ - 0.4f;
  if (mouse_position_.x() + dx < 0.0f)
    new_x = 0.4f;
  if (mouse_position_.y() + dy >= screen_height_)
    new_y = screen_height_ - 0.4f;
  if (mouse_position_.y() + dy < 0.0f)
    new_y = 0.4f;
  last_mouse_position_ = mouse_position_;
  mouse_position_ = base::geometry::FloatPoint(new_x, new_y);
}


}  // namespace wm
}  // namespace naive
