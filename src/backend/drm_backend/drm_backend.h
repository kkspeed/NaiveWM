#ifndef BACKEND_DRM_BACKEND_DRM_BACKEND_H_
#define BACKEND_DRM_BACKEND_DRM_BACKEND_H_

#include <memory>

#include "backend/backend.h"
#include "backend/egl_context.h"
#include "wayland/display_metrics.h"

namespace naive {
namespace base {
class Looper;
}  // namespace base

namespace event {
class EventHub;
}  // namespace event

namespace backend {

class DrmBackend : public Backend {
 public:
  DrmBackend();
  ~DrmBackend() = default;

  // Backend overrides
  bool SupportHwCursor() override { return true; }
  void* PointerData() override { return cursor_data_; }
  void MoveCursor(int32_t x, int32_t y) override;
  void FinalizeDraw(bool did_draw) override;
  EglContext* egl() override { return egl_.get(); }
  wayland::DisplayMetrics* display_metrics() override {
    return display_metrics_.get();
  }
  event::EventHub* GetEventHub() override { return event_hub_.get(); }
  void AddHandler(base::Looper* handler) override;

 private:
  void* cursor_data_;
  std::unique_ptr<EglContext> egl_;
  std::unique_ptr<wayland::DisplayMetrics> display_metrics_;
  std::unique_ptr<event::EventHub> event_hub_;
};

}  // namespace backend
}  // namespace naive

#endif  // BACKEND_DRM_BACKEND_DRM_BACKEND_H_
