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
  UpdateKeyStates(key_event);

  if (!key_event->window()) {
    if (target_)
      wl_keyboard_send_leave(resource_, next_serial(), target_->resource());
    return;
  }

  if (!CanReceiveEvent(key_event->window()->surface()))
    return;

  if (key_event->window()->surface() != target_) {
    if (target_)
      wl_keyboard_send_leave(resource_, next_serial(), target_->resource());
    target_ = key_event->window()->surface();
    // TODO: Keys!
    wl_keyboard_send_enter(resource_,
                           next_serial(),
                           target_->resource(),
                           nullptr);
    return;
  }
  if (key_event->keycode() == 0) {
    // TODO: construct modifiers.
    wl_keyboard_send_modifiers(resource_, next_serial(), 0, 0, 0, 0);
    return;
  } else {
    wl_keyboard_send_key(
        resource_, next_serial(), key_event->time(),
        key_event->keycode(),
        key_event->pressed() ? WL_KEYBOARD_KEY_STATE_PRESSED
                             : WL_KEYBOARD_KEY_STATE_RELEASED);
  }
}

void Keyboard::OnSurfaceDestroyed(Surface* surface) {
  TRACE("%p", surface);
  if (surface == target_)
    target_ = nullptr;
}

void Keyboard::UpdateKeyStates(wm::KeyboardEvent* key_event) {
  if (key_event->keycode() != 0) {
    if (key_event->pressed())
      pressed_keys_.insert(key_event->keycode());
    else
      pressed_keys_.erase(key_event->keycode());
  }
}

uint32_t Keyboard::next_serial() {
  return wl_display_next_serial(
      wl_client_get_display(wl_resource_get_client(resource_)));
}

}  // namespace wayland
}  // namespace naive
