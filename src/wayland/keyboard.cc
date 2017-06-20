#include "wayland/keyboard.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "base/logging.h"
#include "wm/keyboard_event.h"

namespace naive {
namespace wayland {

Keyboard::Keyboard(wl_resource* resource)
    : resource_(resource),
      xkb_context_(xkb_context_new(XKB_CONTEXT_NO_FLAGS)) {
  TRACE();
  // TODO: maybe reconstruct this in SendLayout()?
  xkb_keymap_ = xkb_keymap_new_from_names(
      xkb_context_, NULL, static_cast<xkb_keymap_compile_flags>(0));
  xkb_state_ = xkb_state_new(xkb_keymap_);
  SendLayout();
  wm::WindowManager::Get()->AddKeyboardObserver(this);
}

Keyboard::~Keyboard() {
  xkb_context_unref(xkb_context_);
  xkb_keymap_unref(xkb_keymap_);
  wm::WindowManager::Get()->RemoveKeyboardObserver(this);
}

void Keyboard::SendLayout() {
  TRACE();
  char* keymap_string = xkb_keymap_get_as_string(xkb_keymap_,
                                                 XKB_KEYMAP_FORMAT_TEXT_V1);
  uint32_t keymap_length = static_cast<uint32_t>(strlen(keymap_string) + 1);
  int fd = shm_open("keymap", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  ftruncate(fd, keymap_length);
  void* area = mmap(NULL,
                    keymap_length,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    fd,
                    0);
  // TODO: call mumap somewhere?
  memcpy(area, keymap_string, keymap_length);
  wl_keyboard_send_keymap(resource_, XKB_KEYMAP_FORMAT_TEXT_V1,
                          fd, keymap_length);
  shm_unlink("keymap");
  free(keymap_string);
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
    wl_array keys;
    wl_array_init(&keys);
    for (auto key : pressed_keys_) {
      uint32_t* value = static_cast<uint32_t*>(
          wl_array_add(&keys, sizeof(uint32_t)));
      *value = key;
    }
    // TODO: Keys!
    wl_keyboard_send_enter(resource_,
                           next_serial(),
                           target_->resource(),
                           &keys);
    wl_array_release(&keys);
    return;
  }
  if (key_event->keycode() == 0) {
    LOG_ERROR << "send modifiers " << key_event->shift_pressed() << std::endl;
    wl_keyboard_send_modifiers(
        resource_, next_serial(),
        xkb_state_serialize_mods(xkb_state_,
                                 static_cast<xkb_state_component>(XKB_STATE_DEPRESSED)),
        xkb_state_serialize_mods(xkb_state_,
                                 static_cast<xkb_state_component>(XKB_STATE_LOCKED)),
        xkb_state_serialize_mods(xkb_state_,
                                 static_cast<xkb_state_component>(XKB_STATE_LATCHED)),
        xkb_state_serialize_layout(xkb_state_, XKB_STATE_LAYOUT_EFFECTIVE));
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
  } else {
    uint32_t xkb_modifiers = 0;
    uint32_t xkb_leds = 0;
    if (key_event->ctrl_pressed())
      xkb_modifiers |=
          1 << xkb_keymap_mod_get_index(xkb_keymap_, XKB_MOD_NAME_CTRL);
    if (key_event->alt_pressed())
      xkb_modifiers |=
          1 << xkb_keymap_mod_get_index(xkb_keymap_, XKB_MOD_NAME_ALT);
    if (key_event->shift_pressed())
      xkb_modifiers |=
          1 << xkb_keymap_mod_get_index(xkb_keymap_, XKB_MOD_NAME_SHIFT);

    if (key_event->caps_lock_on())
      xkb_leds |= 1 << xkb_keymap_led_get_index(xkb_keymap_, XKB_LED_NAME_CAPS);
    if (key_event->num_lock_on())
      xkb_leds |= 1 << xkb_keymap_led_get_index(xkb_keymap_, XKB_LED_NAME_NUM);
    if (key_event->scroll_lock_on())
      xkb_leds |=
          1 << xkb_keymap_led_get_index(xkb_keymap_, XKB_LED_NAME_SCROLL);
    xkb_state_update_mask(xkb_state_, xkb_modifiers, 0, xkb_leds, 0, 0, 0);
  }
}

uint32_t Keyboard::next_serial() {
  return wl_display_next_serial(
      wl_client_get_display(wl_resource_get_client(resource_)));
}

}  // namespace wayland
}  // namespace naive
