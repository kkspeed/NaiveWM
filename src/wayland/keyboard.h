#ifndef WAYLAND_KEYBOARD_H_
#define WAYLAND_KEYBOARD_H_

#include <cstdint>
#include <set>
#include <wayland-server.h>
#include <xkbcommon/xkbcommon.h>

#include "compositor/surface.h"
#include "wm/window_manager.h"

namespace naive {
namespace wayland {

class Keyboard: public wm::KeyboardObserver,
                public SurfaceObserver {
 public:
  explicit Keyboard(wl_resource* resource);
  ~Keyboard();

  // KeyboardObserver overrides
  void OnKey(wm::KeyboardEvent* key_event) override;

  // SurfaceObserver overrides
  void OnSurfaceDestroyed(Surface* surface) override;

  bool CanReceiveEvent(Surface* surface);
 private:
  void UpdateKeyStates(wm::KeyboardEvent* key_event);
  void SendLayout();
  uint32_t next_serial();

  xkb_context* xkb_context_;
  xkb_keymap* xkb_keymap_;
  xkb_state* xkb_state_;
  wl_resource* resource_;
  std::set<uint32_t> pressed_keys_;
  Surface* target_ = nullptr;
};

}  // namespace wayland
}  // namespace naive

#endif  // WAYLAND_KEYBOARD_H_
