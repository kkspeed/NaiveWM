#include "wayland/keyboard.h"

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

}  // namespace wayland
}  // namespace naive
