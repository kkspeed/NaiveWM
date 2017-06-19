#ifndef WM_EVENT_H_
#define WM_EVENT_H_

#include <cstdint>

namespace naive {
namespace wm {

class Window;

class Event {
 public:
  Event(Window* window, uint32_t time, uint32_t modifiers)
      : window_(window), time_(time) {}

  virtual Window* window() { return window_; }
  virtual uint32_t time() { return time_; }
 private:
  Window* window_;
  uint32_t time_;
};

}  // namespace wm
}  // namespace naive

#endif  // WM_EVENT_H_
