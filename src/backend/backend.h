#ifndef BACKEND_BACKEND_H_
#define BACKEND_BACKEND_H_

#include "base/looper.h"

namespace naive {
namespace wayland {
class DisplayMetrics;
}  // namespace wayland

namespace event {
class EventHub;
}  // namespace event

namespace backend {
class EglContext;

class Backend {
 public:
  virtual bool SupportHwCursor() { return false; }
  virtual void* PointerData() = 0;
  virtual void MoveCursor(int32_t x, int32_t y) {}
  virtual void FinalizeDraw(bool did_draw) = 0;
  virtual EglContext* egl() = 0;
  virtual wayland::DisplayMetrics* display_metrics() = 0;
  virtual event::EventHub* GetEventHub() = 0;
  virtual void AddHandler(base::Looper* handler) = 0;
};

}  // namespace backend
}  // namespace naive

#endif  // BACKEND_BACKEND_H_
