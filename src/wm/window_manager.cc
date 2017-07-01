#include "window_manager.h"

#include <algorithm>
#include <cassert>
#include <compositor/compositor.h>

#include "base/geometry.h"
#include "base/logging.h"
#include "base/time.h"
#include "wm/keyboard_event.h"
#include "wm/mouse_event.h"

namespace naive {
namespace wm {

namespace {

void CollectGlobalLayers(std::vector<std::unique_ptr<Layer>> &accumulator,
                         Window* window,
                         int32_t start_x,
                         int32_t start_y) {
  // When a window is not visible, its descendants are all considered hidden.
  if (!window->is_visible())
    return;
  accumulator.push_back(std::make_unique<Layer>(window, start_x, start_y));
  for (auto* w : window->children())
    CollectGlobalLayers(accumulator, w, start_x + w->geometry().x(),
                        start_y + w->geometry().y());
}

}  // namespace

// static
WindowManager* WindowManager::g_window_manager = nullptr;

// static
void WindowManager::InitializeWindowManager(
    WmEventObserver* wm_event_observer) {
  g_window_manager = new WindowManager(wm_event_observer);
}

WindowManager* WindowManager::Get() {
  assert(g_window_manager != nullptr);
  return g_window_manager;
}

// TODO: use real dimension
WindowManager::WindowManager(WmEventObserver* wm_event_observer)
    : wm_event_observer_(wm_event_observer) {
  int32_t metrics[2];
  compositor::Compositor::Get()->GetDisplayMetrics(metrics);
  screen_width_ = metrics[0];
  screen_height_ = metrics[1];
  mouse_position_ =
      base::geometry::FloatPoint(screen_width_ / 2, screen_height_ / 2);
  last_mouse_position_ = mouse_position_;
  wm_event_observer_->set_wm_primitives(this);
  wm_event_observer_->set_workspace_dimension(screen_width_, screen_height_);
  event::EventHub::Get()->AddEventObserver(this);
}

void WindowManager::Manage(Window* window) {
  TRACE("manage: %p", window);
  // TODO: window management policy
  auto it = std::find(windows_.begin(), windows_.end(), window);
  if (it != windows_.end()) {
    TRACE("shouldn't happen. %p is already managed.", window);
    return;
  }
  windows_.push_back(window);
  window->set_managed(true);
  // We'll skip popup windows for configure.
  if (window->is_popup() || window->is_transient())
    return;
  wm_event_observer_->WindowCreated(window);
}

void WindowManager::RemoveWindow(Window* window) {
  auto* top_level = window->top_level();
  if (top_level == window) {
    // We are dealing with a top level window
    auto iter = std::find(windows_.begin(), windows_.end(), window);
    if (iter != windows_.end()) {
      if (*iter == focused_window_) {
        focused_window_->LoseFocus();
        auto* next_window = NextWindow(focused_window_);
        FocusWindow(next_window);
      }
      wm_event_observer_->WindowDestroying(window);
      windows_.erase(iter);
      wm_event_observer_->WindowDestroyed(window);
      // TODO: compositor needs to be notified of this
    }
  } else {
    if (window == focused_window_) {
      LOG_ERROR << "set focus to parent" << std::endl;
      window->LoseFocus();
      FocusWindow(window->parent());
      wm_event_observer_->WindowDestroyed(window);
    }
    if (window == global_grab_window_)
      global_grab_window_ = nullptr;
  }
}

void WindowManager::AddMouseObserver(MouseObserver* observer) {
  if (std::find(mouse_observers_.begin(), mouse_observers_.end(), observer) ==
      mouse_observers_.end()) {
    mouse_observers_.push_back(observer);
  }
}

void WindowManager::RemoveMouseObserver(MouseObserver* observer) {
  auto iter =
      std::find(mouse_observers_.begin(), mouse_observers_.end(), observer);
  if (iter != mouse_observers_.end())
    mouse_observers_.erase(iter);
}

void WindowManager::AddKeyboardObserver(KeyboardObserver* observer) {
  if (std::find(keyboard_observers_.begin(), keyboard_observers_.end(),
                observer) == keyboard_observers_.end()) {
    keyboard_observers_.push_back(observer);
  }
}

void WindowManager::RemoveKeyboardObserver(KeyboardObserver* observer) {
  auto iter = std::find(keyboard_observers_.begin(), keyboard_observers_.end(),
                        observer);
  if (iter != keyboard_observers_.end())
    keyboard_observers_.erase(iter);
}

void WindowManager::GlobalGrabWindow(Window* window) {
  // TODO: sublevel menu grab..
  // if (global_grab_window_)
  // global_grab_window_->GrabDone();

  global_grab_window_ = window;
}

bool WindowManager::pointer_moved() {
  return last_mouse_position_.manhattan_distance(mouse_position_) >= 1.0;
}

void WindowManager::OnMouseButton(uint32_t button,
                                  bool pressed,
                                  uint32_t modifiers,
                                  event::Leds leds) {
  MouseEventData data;
  data.button = button;
  DispatchMouseEvent(std::make_unique<MouseEvent>(
      FindMouseEventTarget(),
      pressed ? MouseEventType::MouseButtonDown : MouseEventType::MouseButtonUp,
      base::Time::CurrentTimeMilliSeconds(), modifiers, data,
      mouse_position_.x(), mouse_position_.y(), leds));
}

void WindowManager::OnMouseMotion(float dx,
                                  float dy,
                                  uint32_t modifiers,
                                  event::Leds leds) {
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
  DispatchMouseEvent(std::make_unique<MouseEvent>(
      FindMouseEventTarget(), MouseEventType::MouseMotion,
      base::Time::CurrentTimeMilliSeconds(), modifiers, data, new_x, new_y,
      leds));
}

void WindowManager::OnMouseScroll(float x_scroll,
                                  float y_scroll,
                                  uint32_t modifiers,
                                  event::Leds locks) {
  MouseEventData data;
  data.scroll[0] = x_scroll;
  data.scroll[1] = y_scroll;
  DispatchMouseEvent(std::make_unique<MouseEvent>(
      FindMouseEventTarget(), MouseEventType::MouseAxis,
      base::Time::CurrentTimeMilliSeconds(), modifiers, data,
      mouse_position_.x(), mouse_position_.y(), locks));
}

void WindowManager::OnKey(uint32_t keycode,
                          uint32_t modifiers,
                          bool key_down,
                          event::Leds locks) {
  auto event = std::make_unique<KeyboardEvent>(
      focused_window(), keycode, locks, base::Time::CurrentTimeMilliSeconds(),
      key_down, modifiers);
  if (wm_event_observer_->OnKey(event.get()))
    return;

  if (focused_window_) {
    for (auto observer : keyboard_observers_)
      observer->OnKey(event.get());
  }
  if (keycode == 0) {
    // TODO: send modifiers event
  }
}

std::vector<std::unique_ptr<Layer>>
WindowManager::WindowsInGlobalCoordinates() {
  std::vector<std::unique_ptr<Layer>> result;
  for (auto* w : windows_)
    CollectGlobalLayers(result, w, w->wm_x(), w->wm_y());
  return result;
}

Window* WindowManager::FindMouseEventTarget() {
  std::vector<std::unique_ptr<Layer>> layers = WindowsInGlobalCoordinates();
  int32_t mouse_x = static_cast<int32_t>(mouse_position_.x());
  int32_t mouse_y = static_cast<int32_t>(mouse_position_.y());

  // TODO: establish popup grab, and send popup done when clicking is outside.
  for (auto it = layers.rbegin(); it != layers.rend(); it++) {
    TRACE("Testing %d %d, window: %p, %d %d %d %d", mouse_x, mouse_y,
          (*it)->window(), (*it)->geometry().x(), (*it)->geometry().y(),
          (*it)->geometry().width(), (*it)->geometry().height());
    if ((*it)->geometry().ContainsPoint(mouse_x, mouse_y))
      return (*it)->window();
  }

  return nullptr;
}

void WindowManager::DispatchMouseEvent(std::unique_ptr<MouseEvent> event) {
  if (wm_event_observer_->OnMouseEvent(event.get()))
    return;

  if (event->window()) {
    if (global_grab_window_ && event->window() != global_grab_window_
        && event->type() == MouseEventType::MouseButtonDown) {
      global_grab_window_->GrabDone();
      global_grab_window_ = nullptr;
    }

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
    // TODO: this is dirty.. try a different approach!
    // Finally, window has a transformation
    coord_x -= window_path.back()->wm_x();
    coord_y -= window_path.back()->wm_y();
    event->set_coordinates(coord_x, coord_y);
    LOG_ERROR << " dispatch final coords: " << coord_x << " " << coord_y
              << std::endl;
    for (auto mouse_observer : mouse_observers_)
      mouse_observer->OnMouseEvent(event.get());
  }
}

Window* WindowManager::NextWindow(Window* window) {
  TRACE();
  auto iter = std::find(windows_.begin(), windows_.end(), window);
  if (iter != windows_.end() && iter + 1 != windows_.end())
    return *(iter + 1);
  if (iter == windows_.end() && windows_.size() > 0)
    return windows_.front();
  return nullptr;
}

Window* WindowManager::PreviousWindow(Window* window) {
  TRACE();
  auto iter = std::find(windows_.begin(), windows_.end(), window);
  if (iter != windows_.begin())
    return *(iter - 1);
  if (iter == windows_.begin() && windows_.size() > 0)
    return windows_.back();
  return nullptr;
}

void WindowManager::FocusWindow(Window* window) {
  TRACE("%p, current_focus: %p", window, focused_window_);
  if (window == focused_window_)
    return;

  if (focused_window_)
    focused_window_->LoseFocus();
  focused_window_ = window;
  for (auto* observer : keyboard_observers_)
    observer->OnFocus(window);
  if (window) {
    window->TakeFocus();
    RaiseWindow(window);
    TRACE("top_level: %p", window->top_level());
  }
}

void WindowManager::MoveResizeWindow(Window* window,
                                     base::geometry::Rect resize) {
  // window->SetPosition(resize.x(), resize.y());
  window->WmSetPosition(resize.x(), resize.y());
  window->WmSetSize(resize.width(), resize.height());
}

void WindowManager::RaiseWindow(Window* window) {
  auto it = std::find(windows_.begin(), windows_.end(), window);
  if (it != windows_.end()) {
    windows_.erase(it);
    windows_.push_back(window);
  }
}

}  // namespace wm
}  // namespace naive
