#ifndef EVENT_EVENT_HUB_H_
#define EVENT_EVENT_HUB_H_

#include <libinput.h>
#include <cstdint>
#include <vector>

namespace naive {
namespace event {

enum KeyModifiers {
  CONTROL = 1 << 0,
  SHIFT = 1 << 1,
  ALT = 1 << 2,
  SUPER = 1 << 3,
};

enum Leds {
  CAPS_LOCK = LIBINPUT_LED_CAPS_LOCK,
  NUM_LOCK = LIBINPUT_LED_NUM_LOCK,
  SCROLL_LOCK = LIBINPUT_LED_SCROLL_LOCK
};

class EventObserver {
 public:
  virtual void OnMouseButton(uint32_t button,
                             bool down,
                             uint32_t modifiers,
                             Leds locks) = 0;
  virtual void OnMouseMotion(float dx,
                             float dy,
                             uint32_t modifiers,
                             Leds locks) = 0;
  virtual void OnMouseScroll(float dx,
                             float dy,
                             uint32_t modifiers,
                             Leds locks) = 0;
  virtual void OnKey(uint32_t keycode,
                     uint32_t modifiers,
                     bool key_down,
                     Leds locks) = 0;
};

// establishes a connection between libevent and window manager.
class EventHub {
 public:
  virtual int GetFileDescriptor() = 0;
  virtual void HandleEvents() = 0;
  virtual void AddEventObserver(EventObserver* observer) = 0;
};

}  // namespace event
}  // namespace naive

#endif  // EVENT_EVENTHUB_H_
