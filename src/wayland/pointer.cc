#include "wayland/pointer.h"

#include <cassert>

namespace naive {
namespace wayland {

Pointer::Pointer(wl_resource* resource)
    : resource_(resource), target_(nullptr) {
  TRACE("pointer ctor %p", this);
  wm::WindowManager::Get()->AddMouseObserver(this);
}

Pointer::~Pointer() {
  TRACE("pointer dtor %p", this);
  for (auto* surface : observing_surfaces_)
    surface->RemoveSurfaceObserver(this);
  wm::WindowManager::Get()->RemoveMouseObserver(this);
}

bool Pointer::CanReceiveEvent(Surface* surface) {
  return wl_resource_get_client(surface->resource()) ==
         wl_resource_get_client(resource_);
}

void Pointer::OnMouseEvent(wm::MouseEvent* event) {
  if (!event->window() ||
      event->window()->window_type() != wm::WindowType::NORMAL) {
    if (target_) {
      TRACE("leave to null surface, pointer resource: %p, current target: %p",
            resource_, target_);
      wl_pointer_send_leave(resource_, next_serial(), target_->resource());
    }
    target_ = nullptr;
    return;
  }
  LOG_ERROR << "Dispatch to window: " << event->window() << " "
            << event->window()->surface() << std::endl;
  Surface* surface = event->window()->surface();
  assert(surface);
  if (CanReceiveEvent(surface)) {
    if (observing_surfaces_.find(surface) == observing_surfaces_.end()) {
      surface->AddSurfaceObserver(this);
      observing_surfaces_.insert(surface);
    }
    if (target_ != surface) {
      LOG_ERROR << "leave surface: " << target_ << std::endl;
      if (target_ && target_->window()) {
        LOG_ERROR << "leave window " << target_->window() << std::endl;
        wl_pointer_send_leave(resource_, next_serial(), target_->resource());
      }
      target_ = surface;
      LOG_ERROR << "enter window " << target_->window() << " " << event->x()
                << " " << event->y() << " " << event->time() << std::endl;
      wl_pointer_send_enter(resource_, next_serial(), target_->resource(),
                            wl_fixed_from_int(event->x()),
                            wl_fixed_from_int(event->y()));
      // TODO: do version check here instead of blindly disable it!
      if (wl_resource_get_version(resource_) >= WL_POINTER_FRAME_SINCE_VERSION)
        wl_pointer_send_frame(resource_);
      return;
    }
    switch (event->type()) {
      case wm::MouseEventType::MouseButtonDown:
        TRACE("send button down BTN: %d, %d, %d", event->get_button(),
              event->x(), event->y());
        wl_pointer_send_button(resource_, next_serial(), event->time(),
                               event->get_button(),
                               WL_POINTER_BUTTON_STATE_PRESSED);
        break;
      case wm::MouseEventType::MouseButtonUp:
        TRACE("send button up BTN: %d, %d, %d", event->get_button(), event->x(),
              event->y());
        wl_pointer_send_button(resource_, next_serial(), event->time(),
                               event->get_button(),
                               WL_POINTER_BUTTON_STATE_RELEASED);
        break;
      case wm::MouseEventType::MouseMotion:
        TRACE("send mouse motion: %d, %d at %u", event->x(), event->y(),
              event->time());
        wl_pointer_send_motion(resource_, event->time(),
                               wl_fixed_from_int(event->x()),
                               wl_fixed_from_int(event->y()));
        break;
      case wm::MouseEventType::MouseAxis: {
        float hscroll, vscroll;
        event->get_scroll(hscroll, vscroll);
        TRACE("send mouse scroll: h: %f v: %f at %u", hscroll, vscroll,
              event->time());
        if (abs(hscroll) > 0.000001)
          wl_pointer_send_axis(resource_, event->time(),
                               WL_POINTER_AXIS_HORIZONTAL_SCROLL,
                               wl_fixed_from_double(hscroll));
        if (abs(vscroll) > 0.000001)
          wl_pointer_send_axis(resource_, event->time(),
                               WL_POINTER_AXIS_VERTICAL_SCROLL,
                               wl_fixed_from_double(vscroll));
        break;
      }
    }
  }
  if (wl_resource_get_version(resource_) >= WL_POINTER_FRAME_SINCE_VERSION)
    wl_pointer_send_frame(resource_);
  // TODO: dispatch mouse event by generating all wayland pointer events.
}

void Pointer::OnSurfaceDestroyed(Surface* surface) {
  // TODO: This is a really ugly solution to notify surface removal
  TRACE("%p", surface);
  if (surface == target_)
    target_ = nullptr;
  auto it = observing_surfaces_.find(surface);
  if (it != observing_surfaces_.end())
    observing_surfaces_.erase(it);
}

uint32_t Pointer::next_serial() {
  return wl_display_next_serial(
      wl_client_get_display(wl_resource_get_client(resource_)));
}

}  // namespace wayland
}  // namespace naive
