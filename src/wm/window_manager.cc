#include "window_manager.h"

#include <algorithm>
#include <cassert>

#include "base/geometry.h"
#include "base/logging.h"
#include "wm/mouse_event.h"

namespace naive {
namespace wm {

namespace {

// TODO: convert pointer location to surface local position.
Window* FindMouseEventTargetChildWindow(Window* root,
                                        int32_t x,
                                        int32_t y) {
  Window* candidate = root;
  for (auto iter = root->children().rbegin();
       iter != root->children().rend();
       iter++) {
    Window* current = *iter;
    if (current->geometry().ContainsPoint(x, y))
      return FindMouseEventTargetChildWindow(candidate, x, y);
  }
  return candidate;
}

}  // namespace

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

void WindowManager::AddMouseObserver(MouseObserver* observer) {
  if (std::find(mouse_observers_.begin(), mouse_observers_.end(), observer) ==
      mouse_observers_.end()) {
    mouse_observers_.push_back(observer);
  }
}

void WindowManager::RemoveMouseObserver(MouseObserver* observer) {
  auto iter = std::find(mouse_observers_.begin(), mouse_observers_.end(),
                        observer);
  if (iter != mouse_observers_.end())
    mouse_observers_.erase(iter);
}

bool WindowManager::pointer_moved() {
  return last_mouse_position_.manhattan_distance(mouse_position_) >= 1.0;
}

void WindowManager::OnMouseButton(uint32_t button, bool pressed) {
  MouseEventData data;
  data.button = button;
  DispatchMouseEvent(
      std::make_unique<MouseEvent>(FindMouseEventTarget(),
                                   pressed ? MouseEventType::MouseButtonDown
                                           : MouseEventType::MouseButtonUp,
                                   0,  // TODO: Time
                                   data));
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

  MouseEventData data;
  data.delta[0] = static_cast<int32_t>(new_x - last_mouse_position_.x());
  data.delta[1] = static_cast<int32_t>(new_y - last_mouse_position_.y());
  DispatchMouseEvent(
      std::make_unique<MouseEvent>(FindMouseEventTarget(),
                                   MouseEventType::MouseMotion,
                                   0,  // TODO: Time
                                   data));
}

Window* WindowManager::FindMouseEventTarget() {
  // TODO: could be simplified to one function!
  int32_t mouse_x = static_cast<int32_t>(mouse_position_.x());
  int32_t mouse_y = static_cast<int32_t>(mouse_position_.y());
  for (auto iter = windows_.rbegin(); iter != windows_.rend(); iter++) {
    auto rect = (*iter)->geometry();
    LOG_ERROR << "Testing rect (" << rect.x() << " "
              << rect.y() << " " << rect.x() + rect.width() << " "
              << rect.y() + rect.height() << ") for (" << mouse_x
              << " " << mouse_y << ")" << std::endl;
    if ((*iter)->geometry().ContainsPoint(mouse_x, mouse_y))
      return FindMouseEventTargetChildWindow(*iter, mouse_x, mouse_y);
  }
  return nullptr;
}

void WindowManager::DispatchMouseEvent(std::unique_ptr<MouseEvent> event) {
  for (auto mouse_observer: mouse_observers_)
    mouse_observer->OnMouseEvent(event.get());
}


}  // namespace wm
}  // namespace naive
