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

  bool CanReceiveEvent();

  // MouseObserver overrides:
  void OnMouseEvent(MouseEvent* event);

 private:
  Surface* target_;
};

}  // namespace wayland
}  // namespace naive

#endif  // namespace WAYLAND_POINTER_H_
