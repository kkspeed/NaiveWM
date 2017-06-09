#ifndef COMPOSITOR_SHELL_SURFACE_H
#define COMPOSITOR_SHELL_SURFACE_H

#include <cstdint>
#include <functional>

#include "base/geometry.h"
#include "wm/window.h"

namespace naive {

class ShellSurface {
 public:
  void Move();
  wm::Window* window();
  void set_close_callback(std::function<void()> callback);
  void set_destroy_callback(std::function<void()> callback);
  void set_configure_callback(std::function<uint32_t(int32_t, int32_t)> callback);
  void SetGeometry(const base::geometry::Rect& rect);
  void AcknowledgeConfigure(uint32_t serial);
};

}  // namespace naive

#endif  // COMPOSITOR_SHELL_SURFACE_H
