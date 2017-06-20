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
                event::Leds leds,
                uint32_t time,
                bool pressed,
                uint32_t modifiers)
      : Event(window, time, modifiers), keycode_(keycode),  leds_(leds),
        pressed_(pressed_) {}

  uint32_t keycode() { return keycode_; }
  bool pressed() { return pressed_; }
  event::Leds leds() { return leds_; }

 private:
  bool pressed_;
  uint32_t keycode_;
  event::Leds leds_;
};

}  // namespace wm
}  // namespace naive

#endif  // namespace WM_KEYBOARD_EVENT_H_
