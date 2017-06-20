#include "wayland/keyboard.h"

#include "base/logging.h"
#include "wm/keyboard_event.h"

namespace naive {
namespace wayland {

Keyboard::Keyboard(wl_resource* resource)
    : resource_(resource),
      xkb_context_(xkb_context_new(XKB_CONTEXT_NO_FLAGS)) {
  wm::WindowManager::Get()->AddKeyboardObserver(this);
}

Keyboard::~Keyboard() {
  xkb_context_unref(xkb_context_);
  wm::WindowManager::Get()->RemoveKeyboardObserver(this);
}

bool Keyboard::CanReceiveEvent(Surface* surface) {
  return wl_resource_get_client(resource_) ==
      wl_resource_get_client(surface->resource());
}

void Keyboard::OnKey(wm::KeyboardEvent* key_event) {
  if (key_event->keycode() == 0) {
    // This is an update event.
  } else {
    if (key_event->pressed())
      pressed_keys_.insert(key_event->keycode());
    else
      pressed_keys_.erase(key_event->keycode());
  }
}

void Keyboard::OnSurfaceDestroyed(Surface* surface) {
  TRACE("%p", surface);
  if (surface == target_)
    target_ = nullptr;
}

uint32_t Keyboard::next_serial() {
  return wl_display_next_serial(
      wl_client_get_display(wl_resource_get_client(resource_)));
}

}  // namespace wayland
}  // namespace naive
