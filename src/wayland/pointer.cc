#include "wayland/pointer.h"

#include "wm/window_manager.h"

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
  if (!event->window())
    return;
  Surface* surface = event->window()->surface();
  if (CanReceiveEvent(surface)) {
    // TODO: dispatch mouse event by generating all wayland pointer events.
  }
}

}  // namespace wayland
}  // namespace naive
