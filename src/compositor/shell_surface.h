#ifndef COMPOSITOR_SHELL_SURFACE_H
#define COMPOSITOR_SHELL_SURFACE_H

#include <cstdint>
#include <functional>

#include "base/geometry.h"
#include "compositor/surface.h"
#include "wm/window.h"

namespace naive {

namespace wm {
class Window;
}  // namespace wm;

class ShellSurface : SurfaceObserver {
 public:
  ShellSurface(Surface* surface);
  ~ShellSurface();

  void Move();
  void SetGeometry(const base::geometry::Rect& rect);
  void SetVisibleRegion(const base::geometry::Rect& rect);
  void AcknowledgeConfigure(uint32_t serial);

  // SurfaceObserver overrides:
  void OnCommit() override;
  void OnSurfaceDestroyed(Surface*) override;

  void set_close_callback(std::function<void()> callback) {
    close_callback_ = callback;
  }
  void set_destroy_callback(std::function<void()> callback) {
    destroy_callback_ = callback;
  }
  void set_configure_callback(
      std::function<uint32_t(int32_t, int32_t)> callback) {
    configure_callback_ = callback;
  }
  void set_ungrab_callback(std::function<void()> callback) {
    ungrab_callback_ = callback;
  }

  void Configure(int32_t width, int32_t height);
  void Close();
  void Ungrab() {
    TRACE("ungrabbing %p", this);
    ungrab_callback_();
    ungrab_callback_ = []() {};
  }

  wm::Window* window() { return window_; }

 private:
  std::function<uint32_t(int32_t, int32_t)> configure_callback_;
  std::function<void()> close_callback_;
  std::function<void()> destroy_callback_;
  std::function<void()> ungrab_callback_ = []() {};

  wm::Window* window_;
  Surface* surface_;
  bool in_configure_ = false;
};

}  // namespace naive

#endif  // COMPOSITOR_SHELL_SURFACE_H
