#include "wayland/pointer.h"

namespace naive {
namespace wayland {

Pointer::Pointer(wl_resource* resource)
    : resource_(resource),
      target_(nullptr) {}

bool Pointer::CanReceiveEvent(Surface* surface) {
  return wl_resource_get_client(surface->resource()) ==
      wl_resource_get_client(resource_);
}

void Pointer::OnMouseEvent(wm::MouseEvent* event) {
  // TODO: dispatch mouse event by generating all wayland pointer events.
}

}  // namespace wayland
}  // namespace naive
