#include "wayland/pointer.h"

#include <cassert>

namespace naive {
namespace wayland {

Pointer::Pointer(wl_resource* resource)
    : resource_(resource),
      target_(nullptr) {
  wm::WindowManager::Get()->AddMouseObserver(this);
}

Pointer::~Pointer() {
  wm::WindowManager::Get()->RemoveMouseObserver(this);
}

bool Pointer::CanReceiveEvent(Surface* surface) {
  return wl_resource_get_client(surface->resource()) ==
      wl_resource_get_client(resource_);
}

void Pointer::OnMouseEvent(wm::MouseEvent* event) {

  if (!event->window()) {
    if (target_)
      wl_pointer_send_leave(resource_, next_serial(), target_->resource());
    target_ = nullptr;
    return;
  }
  LOG_ERROR << "Dispatch to window: " << event->window() << " "
      << event->window()->surface() << std::endl;
  Surface* surface = event->window()->surface();
  assert(surface);
  if (CanReceiveEvent(surface)) {
    if (target_ != surface) {
      LOG_ERROR << "Send leave " << std::endl;
      if (target_)
        wl_pointer_send_leave(resource_, next_serial(), target_->resource());
      target_ = surface;
      // TODO: calculate pointer location
      wl_pointer_send_enter(resource_,
                            next_serial(),
                            surface->resource(),
                            0,
                            0);
    }
    switch (event->type()) {
      case wm::MouseEventType::MouseButtonDown:
        LOG_ERROR << "Send Button Down " << event->get_button() << std::endl;
        wl_pointer_send_button(resource_, next_serial(), event->time(),
                               event->get_button(),
                               WL_POINTER_BUTTON_STATE_PRESSED);
        break;
      case wm::MouseEventType::MouseButtonUp:
        LOG_ERROR << "Send Button Up " << event->get_button() << std::endl;
        wl_pointer_send_button(resource_, next_serial(), event->time(),
                               event->get_button(),
                               WL_POINTER_BUTTON_STATE_RELEASED);
        break;
      default:
        // TODO: Handle other mouse events.
        break;
    }
    // TODO: dispatch mouse event by generating all wayland pointer events.
  }
}

uint32_t Pointer::next_serial() {
  return wl_display_next_serial(
      wl_client_get_display(wl_resource_get_client(resource_)));
}

}  // namespace wayland
}  // namespace naive
