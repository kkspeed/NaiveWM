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
  int32_t delta[2];  // for axis and motion.
  int32_t button;  // for button down and up.
};

class MouseEvent: public Event {
 public:
  MouseEvent(Window* window, MouseEventType type, uint32_t time,
             MouseEventData data)
      : window_(window), type_(type), time_(time), data_(data) {}

  Window* window() { return window_; }
  MouseEventType type() { return type_; }
  uint32_t time() { return time_; }

  void get_delta(int32_t& dx, int32_t dy) {
    dx = data_.delta[0];
    dy = data_.delta[1];
  }

  int get_button() {
    return data_.button;
  }
 private:
  Window* window_;
  MouseEventType type_;
  uint32_t time_;
  MouseEventData data_;
};

}  // namespace wm
}  // namespace naive

#endif  //NAIVEWM_MOUSE_EVENT_H
