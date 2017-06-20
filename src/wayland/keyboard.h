#ifndef WAYLAND_KEYBOARD_H_
#define WAYLAND_KEYBOARD_H_

#include <wayland-server.h>
#include <xkbcommon/xkbcommon.h>

#include "wm/window_manager.h"

namespace naive {
namespace wayland {

class Keyboard: public wm::KeyboardObserver {
 public:
  explicit Keyboard(wl_resource* resource);
  ~Keyboard();

  // KeyboardObserver overrides
  void OnKey();

 private:
  xkb_context* xkb_context_;
  wl_resource* resource_;
  std::vector<uint32_t> pressed_keys_;
};

}  // namespace wayland
}  // namespace naive

#endif  // WAYLAND_KEYBOARD_H_
