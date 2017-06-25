#ifndef WM_MOUSE_EVENT_H_
#define WM_MOUSE_EVENT_H_

#include <cstdint>

#include "wm/event.h"

namespace naive {
namespace wm {

class Window;

enum class MouseEventType {
  MouseAxis,
  MouseButtonDown,
  MouseButtonUp,
  MouseMotion
};

union MouseEventData {
  int32_t delta[2];  // for motion.
  float scroll[2];   // for scroll.
  uint32_t button;   // for button down and up.
};

class MouseEvent : public Event {
 public:
  MouseEvent(Window* window,
             MouseEventType type,
             uint32_t time,
             uint32_t modifiers,
             MouseEventData data,
             uint32_t x,
             uint32_t y,
             event::Leds locks)
      : Event(window, time, modifiers, locks),
        type_(type),
        data_(data),
        x_(x),
        y_(y) {}

  MouseEventType type() { return type_; }

  void get_delta(int32_t& dx, int32_t& dy) {
    dx = data_.delta[0];
    dy = data_.delta[1];
  }

  void get_scroll(float& hscroll, float& vscroll) {
    hscroll = data_.scroll[0];
    vscroll = data_.scroll[1];
  }

  uint32_t get_button() { return data_.button; }

  void set_coordinates(uint32_t x, uint32_t y) {
    x_ = x;
    y_ = y;
  }

  uint32_t x() { return x_; }
  uint32_t y() { return y_; }

 private:
  MouseEventType type_;
  MouseEventData data_;
  uint32_t x_, y_;
};

}  // namespace wm
}  // namespace naive

#endif  // NAIVEWM_MOUSE_EVENT_H
