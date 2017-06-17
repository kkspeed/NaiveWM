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
  } else {
    LOG_ERROR << "Dispatch to window: " << event->window() << " "
              << event->window()->surface() << std::endl;
    Surface* surface = event->window()->surface();
    assert(surface);
    if (CanReceiveEvent(surface)) {
      if (target_ != surface) {
        if (target_) {
          wl_pointer_send_leave(resource_, next_serial(), target_->resource());
          LOG_ERROR << "leave window " << target_->window() << std::endl;
        }
        target_ = surface;
        LOG_ERROR << "enter window " << target_->window() << std::endl;
        wl_pointer_send_enter(resource_,
                              next_serial(),
                              surface->resource(),
                              event->x(),
                              event->y());
      }
      switch (event->type()) {
        case wm::MouseEventType::MouseButtonDown:TRACE(
              "send button down BTN: %d, %d, %d",
              event->get_button(),
              event->x(),
              event->y());
          wl_pointer_send_button(resource_, next_serial(), event->time(),
                                 event->get_button(),
                                 WL_POINTER_BUTTON_STATE_PRESSED);
          break;
        case wm::MouseEventType::MouseButtonUp:TRACE(
              "send button up BTN: %d, %d, %d",
              event->get_button(),
              event->x(),
              event->y());
          wl_pointer_send_button(resource_, next_serial(), event->time(),
                                 event->get_button(),
                                 WL_POINTER_BUTTON_STATE_RELEASED);
          break;
        case wm::MouseEventType::MouseMotion:TRACE("send mouse motion: %d, %d",
                                                   event->x(),
                                                   event->y());
          wl_pointer_send_motion(resource_, next_serial(), event->x(),
                                 event->y());
          break;
        default:
          // TODO: Handle other mouse events.
          break;
      }
    }
    wl_pointer_send_frame(resource_);
    // TODO: dispatch mouse event by generating all wayland pointer events.
  }
}

uint32_t Pointer::next_serial() {
  return wl_display_next_serial(
      wl_client_get_display(wl_resource_get_client(resource_)));
}

}  // namespace wayland
}  // namespace naive
