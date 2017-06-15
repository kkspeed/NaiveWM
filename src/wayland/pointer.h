#ifndef WAYLAND_POINTER_H_
#define WAYLAND_POINTER_H_

#include <wayland-server.h>

#include "wm/window_manager.h"

namespace naive {

namespace wm {
class MouseEvent;
}  // namespace wm

class Surface;

namespace wayland {

class Pointer:  wm::MouseObserver {
 public:
  explicit Pointer(wl_resource* resource);
  ~Pointer();

  bool CanReceiveEvent(Surface* surface);

  // MouseObserver overrides:
  void OnMouseEvent(wm::MouseEvent* event);

 private:
  uint32_t next_serial();

  wl_resource* resource_;
  Surface* target_;
};

}  // namespace wayland
}  // namespace naive

#endif  // namespace WAYLAND_POINTER_H_
