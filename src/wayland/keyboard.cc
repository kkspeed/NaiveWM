#include "wayland/keyboard.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "base/logging.h"
#include "wayland/seat.h"
#include "wm/keyboard_event.h"

namespace naive {
namespace wayland {

// static
bool Keyboard::global_grab_activated_ = false;
// static
void Keyboard::ActivateGlobalGrab(bool activate) {
  global_grab_activated_ = activate;
}

Keyboard::Keyboard(wl_resource* resource, Seat* seat)
    : resource_(resource),
      xkb_context_(xkb_context_new(XKB_CONTEXT_NO_FLAGS)),
      seat_(seat) {
  TRACE("%p", this);
  // TODO: maybe reconstruct this in SendLayout()?
  xkb_keymap_ = xkb_keymap_new_from_names(
      xkb_context_, NULL, static_cast<xkb_keymap_compile_flags>(0));
  xkb_state_ = xkb_state_new(xkb_keymap_);
  SendLayout();
  seat->RegisterKeyboard(wl_resource_get_client(resource), this);
  wm::WindowManager::Get()->AddKeyboardObserver(this);
}

Keyboard::~Keyboard() {
  TRACE("%p", this);
  if (grabbing_)
    grabbing_->Grab(nullptr);
  if (grab_)
    grab_->SetGrabbing(nullptr);
  xkb_context_unref(xkb_context_);
  xkb_keymap_unref(xkb_keymap_);
  wm::WindowManager::Get()->RemoveKeyboardObserver(this);
  for (auto* surface : observed_surfaces_)
    surface->RemoveSurfaceObserver(this);
  if (grab_target_)
    grab_target_->RemoveSurfaceObserver(this);
  seat_->NotifyKeyboardDestroyed(this);
}

void Keyboard::SendLayout() {
  TRACE();
  char* keymap_string =
      xkb_keymap_get_as_string(xkb_keymap_, XKB_KEYMAP_FORMAT_TEXT_V1);
  uint32_t keymap_length = static_cast<uint32_t>(strlen(keymap_string) + 1);
  int fd = shm_open("keymap", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  ftruncate(fd, keymap_length);
  void* area =
      mmap(NULL, keymap_length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  // TODO: call mumap somewhere?
  memcpy(area, keymap_string, keymap_length);
  munmap(area, keymap_length);
  wl_keyboard_send_keymap(resource_, XKB_KEYMAP_FORMAT_TEXT_V1, fd,
                          keymap_length);
  shm_unlink("keymap");
  free(keymap_string);
}

bool Keyboard::CanReceiveEvent(Surface* surface) {
  return wl_resource_get_client(resource_) ==
         wl_resource_get_client(surface->resource());
}

void Keyboard::OnFocus(wm::Window* window) {
  TRACE("%p, target: %p", window, target_);
  if (!window || window->window_type() != wm::WindowType::NORMAL) {
    seat_->NotifyKeyboardFocusChanged(nullptr);
    return;
  }
  if (CanReceiveEvent(window->surface()))
    seat_->NotifyKeyboardFocusChanged(this);
  if (window && window->surface() != target_ &&
      CanReceiveEvent(window->surface())) {
    TRACE("Adding %p as surface observer to %p", this, window->surface());
    if (observed_surfaces_.find(window->surface()) ==
        observed_surfaces_.end()) {
      window->surface()->AddSurfaceObserver(this);
      observed_surfaces_.insert(window->surface());
    }
    if (target_) {
      TRACE("sending leave to %p", target_);
      wl_keyboard_send_leave(resource_, next_serial(), target_->resource());
    }
    target_ = window->surface();
    wl_array keys;
    wl_array_init(&keys);
    for (auto key : pressed_keys_) {
      uint32_t* value =
          static_cast<uint32_t*>(wl_array_add(&keys, sizeof(uint32_t)));
      *value = key;
    }
    wl_keyboard_send_enter(resource_, next_serial(), target_->resource(),
                           &keys);
    wl_array_release(&keys);
    return;
  }
}

void Keyboard::OnKey(wm::KeyboardEvent* key_event) {
  TRACE();
  UpdateKeyStates(key_event);

  if (!grab_target_ && global_grab_activated_)
    return;

  if (!grab_target_) {
    if (!key_event->window() ||
        key_event->window()->window_type() != wm::WindowType::NORMAL) {
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
      TRACE("Adding %p as surface observer to %p", this, target_);
      if (observed_surfaces_.find(target_) == observed_surfaces_.end()) {
        target_->AddSurfaceObserver(this);
        observed_surfaces_.insert(target_);
      }
      wl_array keys;
      wl_array_init(&keys);
      for (auto key : pressed_keys_) {
        uint32_t* value =
            static_cast<uint32_t*>(wl_array_add(&keys, sizeof(uint32_t)));
        *value = key;
      }
      seat_->NotifyKeyboardFocusChanged(this);
      wl_keyboard_send_enter(resource_, next_serial(), target_->resource(),
                             &keys);
      wl_array_release(&keys);
      return;
    }
  }

  wl_resource* target_keyboard = grab_ ? grab_->resource() : resource_;
  // if (key_event->keycode() == 0) {
  //  LOG_ERROR << "send modifiers " << key_event->shift_pressed() <<
  //  std::endl;
  wl_keyboard_send_modifiers(
      target_keyboard, next_serial(),
      xkb_state_serialize_mods(
          xkb_state_, static_cast<xkb_state_component>(XKB_STATE_DEPRESSED)),
      xkb_state_serialize_mods(
          xkb_state_, static_cast<xkb_state_component>(XKB_STATE_LOCKED)),
      xkb_state_serialize_mods(
          xkb_state_, static_cast<xkb_state_component>(XKB_STATE_LATCHED)),
      xkb_state_serialize_layout(xkb_state_, XKB_STATE_LAYOUT_EFFECTIVE));
  //  return;
  //} else {
  wl_keyboard_send_key(target_keyboard, next_serial(), key_event->time(),
                       key_event->keycode(),
                       key_event->pressed() ? WL_KEYBOARD_KEY_STATE_PRESSED
                                            : WL_KEYBOARD_KEY_STATE_RELEASED);
  //}
}

void Keyboard::OnSurfaceDestroyed(Surface* surface) {
  TRACE("%p", surface);
  if (surface == target_)
    target_ = nullptr;
  if (observed_surfaces_.find(surface) != observed_surfaces_.end())
    observed_surfaces_.erase(surface);
  if (grab_target_ == surface)
    grab_target_ = nullptr;
}

void Keyboard::Grab(Keyboard* grab, bool from_set_grabbing) {
  if (grab_ == grab)
    return;
  if (grab_ && !from_set_grabbing)
    grab_->SetGrabbing(nullptr, true);
  grab_ = grab;
  if (grab)
    grab->SetGrabbing(this, true);
}

void Keyboard::SetGrabbing(Keyboard* grabbing, bool from_grab) {
  if (grabbing_ == grabbing)
    return;
  if (grabbing_ && !from_grab)
    grabbing_->Grab(nullptr, true);
  grabbing_ = grabbing;
}

void Keyboard::UpdateKeyStates(wm::KeyboardEvent* key_event) {
  TRACE();
  // if (key_event->keycode() != 0) {
  if (key_event->pressed())
    pressed_keys_.insert(key_event->keycode());
  else
    pressed_keys_.erase(key_event->keycode());
  //} else {
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
    xkb_leds |= 1 << xkb_keymap_led_get_index(xkb_keymap_, XKB_LED_NAME_SCROLL);
  xkb_state_update_mask(xkb_state_, xkb_modifiers, 0, xkb_leds, 0, 0, 0);
  //}
}

uint32_t Keyboard::next_serial() {
  return wl_display_next_serial(
      wl_client_get_display(wl_resource_get_client(resource_)));
}

}  // namespace wayland
}  // namespace naive
