#ifndef COMPOSITOR_SHELL_SURFACE_H
#define COMPOSITOR_SHELL_SURFACE_H

#include <cstdint>
#include <functional>

#include "base/geometry.h"
#include "wm/window.h"

namespace naive {

namespace wm {
class Window;
}  // namespace wm;

class Surface;

class ShellSurface {
 public:
  ShellSurface(Surface* surface);

  void Move();
  void SetGeometry(const base::geometry::Rect& rect);
  void AcknowledgeConfigure(uint32_t serial);

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

  void Configure(int32_t width, int32_t height);
  wm::Window* window() { return window_; }

 private:
  std::function<uint32_t(int32_t, int32_t)> configure_callback_;
  std::function<void()> close_callback_;
  std::function<void()> destroy_callback_;

  wm::Window* window_;
  Surface* surface_;
};

}  // namespace naive

#endif  // COMPOSITOR_SHELL_SURFACE_H
