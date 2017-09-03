#ifndef BACKEND_X11_BACKEND_X11_BACKEND_H_
#define BACKEND_X11_BACKEND_X11_BACKEND_H_

#include <X11/Xlib.h>
#include <memory>
#include <vector>

#include "base/logging.h"
#include "backend/backend.h"
#include "event/event_hub.h"

namespace naive {

namespace wayland {
class DisplayMetrics;
}  // namespace wayland

namespace backend {
class EglContext;

class X11Backend: public Backend, public event::EventHub {
public:
  X11Backend(const char* display);
  ~X11Backend() = default;

  // EventHub overrides.
  int GetFileDescriptor() override {
    TRACE("X11Backend does not provide file descriptor");
    return -1;
  }
  void HandleEvents() override { TRACE("Should not be called."); }
  void AddEventObserver(event::EventObserver* observer) override {
    observers_.push_back(observer);
  }

  void FinalizeDraw(bool did_draw) override;
  EglContext* egl() override { return egl_.get(); }
  wayland::DisplayMetrics* display_metrics() override { return display_metrics_.get(); }
  event::EventHub* GetEventHub() override { return this; }
  void AddHandler(base::Looper* handler) override;
private:
  void DispatchEvents();
  void HandleKeyEvent(XKeyEvent* event);
  void HandleButtonEvent(XButtonEvent* event);
  void HandleMotionEvent(XMotionEvent* event);
  uint32_t GetButton(uint32_t button);
  uint32_t GetModifiers(uint32_t state);

  Display* x_display_;
  Window x_window_;
  std::unique_ptr<EglContext> egl_;
  std::unique_ptr<wayland::DisplayMetrics> display_metrics_;
  std::vector<event::EventObserver*> observers_;
  uint32_t last_mouse_x_, last_mouse_y_;
};

}  // backend
}  // namespace naive

#endif  // BACKEND_X11_BACKEND_X11_BACKEND_H_
