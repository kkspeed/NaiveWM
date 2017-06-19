#include "wayland/pointer.h"

#include <cassert>

namespace naive {
namespace wayland {

Pointer::Pointer(wl_resource* resource)
    : resource_(resource),
      target_(nullptr) {
  TRACE();
  wm::WindowManager::Get()->AddMouseObserver(this);
}

Pointer::~Pointer() {
  TRACE("my pointer dtor %p", this);
  if (target_)
    target_->RemoveSurfaceObserver(this);
  wm::WindowManager::Get()->RemoveMouseObserver(this);
}

bool Pointer::CanReceiveEvent(Surface* surface) {
  return wl_resource_get_client(surface->resource()) ==
      wl_resource_get_client(resource_);
}

void Pointer::OnMouseEvent(wm::MouseEvent* event) {

  if (!event->window()) {
    if (target_) {
      LOG_ERROR << "Leave to null surface " << resource_  << " "
                << target_ << std::endl;
      wl_pointer_send_leave(resource_, next_serial(), target_->resource());
    }
    target_ = nullptr;
  } else {
    LOG_ERROR << "Dispatch to window: " << event->window() << " "
              << event->window()->surface() << std::endl;
    Surface* surface = event->window()->surface();
    assert(surface);
    if (CanReceiveEvent(surface)) {
      surface->AddSurfaceObserver(this);
      if (target_ != surface) {
        LOG_ERROR << "leave surface: " << target_ << std::endl;
        if (target_ && target_->window()) {
          LOG_ERROR << "leave window " << target_->window() << std::endl;
          wl_pointer_send_leave(resource_, next_serial(), target_->resource());
        }
        target_ = surface;
        LOG_ERROR << "enter window " << target_->window()
                  << " " << event->x() << " " << event->y()
                  << " " << event->time() << std::endl;
        wl_pointer_send_enter(resource_,
                              next_serial(),
                              target_->resource(),
                              wl_fixed_from_int(event->x()),
                              wl_fixed_from_int(event->y()));
        wl_pointer_send_frame(resource_);
        return;
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
        case wm::MouseEventType::MouseMotion:
          TRACE("send mouse motion: %d, %d at %u",
                event->x(),
                event->y(),
                event->time());
          wl_pointer_send_motion(resource_, event->time(),
                                 wl_fixed_from_int(event->x()),
                                 wl_fixed_from_int(event->y()));
          break;
        default:
          // TODO: Handle other mouse events.
          break;
      }
    }
    wl_pointer_send_frame(resource_);
    LOG_ERROR << "send frame" << std::endl;
    // TODO: dispatch mouse event by generating all wayland pointer events.
  }
}

void Pointer::OnSurfaceDestroyed(Surface* surface) {
  // TODO: This is a really ugly solution to notify surface removal
  TRACE("%p", surface);
  if (surface == target_)
    target_ = nullptr;
}

uint32_t Pointer::next_serial() {
  return wl_display_next_serial(
      wl_client_get_display(wl_resource_get_client(resource_)));
}

}  // namespace wayland
}  // namespace naive
