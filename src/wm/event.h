#ifndef WM_EVENT_H_
#define WM_EVENT_H_

#include <cstdint>

#include "event/event_hub.h"

namespace naive {
namespace wm {

class Window;

class Event {
 public:
  Event(Window* window, uint32_t time, uint32_t modifiers, event::Leds locks)
      : window_(window), time_(time), modifiers_(modifiers), locks_(locks) {}

  Window* window() { return window_; }
  uint32_t time() { return time_; }

  bool caps_lock_on() { return (locks_ & event::Leds::CAPS_LOCK) != 0; }
  bool num_lock_on() { return (locks_ & event::Leds::NUM_LOCK) != 0; }
  bool scroll_lock_on() { return (locks_ & event::Leds::SCROLL_LOCK) != 0; }

  bool ctrl_pressed() {
    return (modifiers_ & event::KeyModifiers::CONTROL) != 0;
  }

  bool alt_pressed() { return (modifiers_ & event::KeyModifiers::ALT) != 0; }

  bool shift_pressed() {
    return (modifiers_ & event::KeyModifiers::SHIFT) != 0;
  }

  bool super_pressed() {
    return (modifiers_ & event::KeyModifiers::SUPER) != 0;
  }

 private:
  Window* window_;
  uint32_t time_;
  uint32_t modifiers_;
  event::Leds locks_;
};

}  // namespace wm
}  // namespace naive

#endif  // WM_EVENT_H_
