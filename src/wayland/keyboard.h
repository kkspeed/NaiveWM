#ifndef WAYLAND_KEYBOARD_H_
#define WAYLAND_KEYBOARD_H_

#include <wayland-server.h>
#include <xkbcommon/xkbcommon.h>
#include <cstdint>
#include <set>

#include "compositor/surface.h"
#include "wm/window_manager.h"

namespace naive {
namespace wayland {

class Seat;

class Keyboard : public wm::KeyboardObserver, public SurfaceObserver {
 public:
  explicit Keyboard(wl_resource* resource, Seat* seat);
  explicit Keyboard(wl_resource* resource, Seat* seat, Surface* surface);
  ~Keyboard();

  // KeyboardObserver overrides
  void OnKey(wm::KeyboardEvent* key_event) override;
  void OnFocus(wm::Window* wincow) override;

  // SurfaceObserver overrides
  void OnSurfaceDestroyed(Surface* surface) override;

  // TODO: this is ugly.. how to avoid infinite recursion?
  void Grab(Keyboard* grab, bool from_set_grabbing = false);
  void SetGrabbing(Keyboard* grabbing, bool from_grab = false);

  bool CanReceiveEvent(Surface* surface);

  wl_resource* resource() { return resource_; }
  Surface* target_surface() { return target_; }

  void set_grab_target(Surface* target) {
    grab_target_ = target;
    if (target)
      target->AddSurfaceObserver(this);
  }

  static void ActivateGlobalGrab(bool activate);

 private:
  void UpdateKeyStates(wm::KeyboardEvent* key_event);
  void SendLayout();
  uint32_t next_serial();

  xkb_context* xkb_context_;
  xkb_keymap* xkb_keymap_;
  xkb_state* xkb_state_;
  wl_resource* resource_;
  std::set<uint32_t> pressed_keys_;
  std::set<Surface*> observed_surfaces_;
  Surface* target_ = nullptr;
  Keyboard* grab_ = nullptr;
  Keyboard* grabbing_ = nullptr;
  Seat* seat_;

  Surface* grab_target_{nullptr};

  static bool global_grab_activated_;
};

}  // namespace wayland
}  // namespace naive

#endif  // WAYLAND_KEYBOARD_H_
