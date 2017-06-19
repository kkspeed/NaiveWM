#include "window_manager.h"

#include <algorithm>
#include <cassert>

#include "base/geometry.h"
#include "base/logging.h"
#include "base/time.h"
#include "wm/mouse_event.h"

namespace naive {
namespace wm {

namespace {

// TODO: convert pointer location to surface local position.
Window* FindMouseEventTargetChildWindow(Window* root,
                                        int32_t x,
                                        int32_t y) {
  TRACE("root window: %p", root);
  Window* candidate = root;
  for (auto iter = root->children().rbegin();
       iter != root->children().rend();
       iter++) {
    Window* current = *iter;
    if (current->geometry().ContainsPoint(x, y))
      return FindMouseEventTargetChildWindow(current, x, y);
  }
  return candidate;
}

}  // namespace

// static
WindowManager* WindowManager::g_window_manager = nullptr;

// static
void WindowManager::InitializeWindowManager(WmEventObserver* wm_event_observer) {
  g_window_manager = new WindowManager(wm_event_observer);
}

WindowManager* WindowManager::Get() {
  assert(g_window_manager != nullptr);
  return g_window_manager;
}

// TODO: use real dimension
WindowManager::WindowManager(WmEventObserver* wm_event_observer) :
    screen_width_(1280), screen_height_(720),
    mouse_position_(1280.0f, 720.0f), last_mouse_position_(1280.0f, 720.0f),
    wm_event_observer_(wm_event_observer) {
  wm_event_observer_->set_wm_primitives(this);
  event::EventHub::Get()->AddEventObserver(this);
}

void WindowManager::Manage(Window* window) {
  TRACE("manage: %p", window);
  // TODO: window management policy
  windows_.push_back(window);
  window->set_managed(true);
  wm_event_observer_->WindowCreated(window);
}

void WindowManager::RemoveWindow(Window* window) {
  auto iter = std::find(windows_.begin(), windows_.end(), window);
  if (iter != windows_.end()) {
    if (*iter == focused_window_) {
      focused_window_->LoseFocus();
      auto* next_window = NextWindow(focused_window_);
      FocusWindow(next_window);
    }
    wm_event_observer_->WindowDestroyed(window);
    windows_.erase(iter);
    // TODO: compositor needs to be notified of this
  }
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
                                   base::Time::CurrentTimeMilliSeconds(),
                                   data,
                                   mouse_position_.x(),
                                   mouse_position_.y()));
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
                                   base::Time::CurrentTimeMilliSeconds(),
                                   data, new_x, new_y));
}

Window* WindowManager::FindMouseEventTarget() {
  // TODO: could be simplified to one function!
  int32_t mouse_x = static_cast<int32_t>(mouse_position_.x());
  int32_t mouse_y = static_cast<int32_t>(mouse_position_.y());

  // We append the focused window to the back of this list for highest priority.
  // during event dispatching.
  std::vector<Window*> temporary_windows(windows_);
  if (focused_window_)
    temporary_windows.push_back(focused_window_);
  for (auto iter = temporary_windows.rbegin(); iter != temporary_windows.rend();
       iter++) {
    if ((*iter)->geometry().ContainsPoint(mouse_x, mouse_y))
      return FindMouseEventTargetChildWindow(*iter, mouse_x, mouse_y);
  }
  return nullptr;
}

void WindowManager::DispatchMouseEvent(std::unique_ptr<MouseEvent> event) {
  if (wm_event_observer_->OnMouseEvent(event.get()))
    return;

  if (event->window()) {
    // Transform pointer location to surface local coordinates.
    std::vector<Window*> window_path;
    Window* window = event->window();
    window_path.push_back(window);
    while (window->parent()) {
      window_path.push_back(window->parent());
      window = window->parent();
    }
    uint32_t coord_x = event->x();
    uint32_t coord_y = event->y();
    for (auto it = window_path.rbegin(); it != window_path.rend(); it++) {
      coord_x = coord_x - (*it)->geometry().x();
      coord_y = coord_y - (*it)->geometry().y();
    }
    event->set_coordinates(coord_x, coord_y);
    for (auto mouse_observer: mouse_observers_)
      mouse_observer->OnMouseEvent(event.get());

  }
}

Window* WindowManager::NextWindow(Window* window) {
  TRACE();
  auto iter = std::find(windows_.begin(), windows_.end(), window);
  if (iter != windows_.end() && iter + 1 != windows_.end())
    return *(iter + 1);
  return nullptr;
}

Window* WindowManager::PreviousWindow(Window* window) {
  TRACE();
  auto iter = std::find(windows_.begin(), windows_.end(), window);
  if (iter != windows_.begin())
    return *(iter - 1);
  return nullptr;
}

void WindowManager::FocusWindow(Window* window) {
  TRACE();
  if (window == focused_window_)
    return;

  if (!window) {
    if (focused_window_)
      focused_window_->LoseFocus();
    focused_window_ = nullptr;
    return;
  }

  auto iter = std::find(windows_.begin(), windows_.end(), window);
  if (iter != windows_.end()) {
    if (focused_window_)
      focused_window_->LoseFocus();
    focused_window_ = *iter;
    if (focused_window_)
      focused_window_->TakeFocus();
    return;
  }

  LOG_ERROR << "window not found!" << std::endl;
}

void WindowManager::MoveResizeWindow(Window* window,
                                     base::geometry::Rect resize) {
  window->SetPosition(resize.x(), resize.y());
  window->WmSetSize(resize.width(), resize.height());
}

}  // namespace wm
}  // namespace naive
