#ifndef WM_KEYBOARD_EVENT_H_
#define WM_KEYBOARD_EVENT_H_

#include "wm/event.h"
#include "event/event_hub.h"

namespace naive {
namespace wm {

class KeyboardEvent : public Event {
 public:
  KeyboardEvent(Window* window,
                uint32_t keycode,
                event::Leds locks,
                uint32_t time,
                bool pressed,
                uint32_t modifiers)
      : Event(window, time, modifiers, locks), keycode_(keycode),
        pressed_(pressed) {}

  uint32_t keycode() { return keycode_; }
  bool pressed() { return pressed_; }

 private:
  bool pressed_;
  uint32_t keycode_;
};

}  // namespace wm
}  // namespace naive

#endif  // namespace WM_KEYBOARD_EVENT_H_
