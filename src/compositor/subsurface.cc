#include "subsurface.h"

namespace naive {

SubSurface::SubSurface(Surface* parent, Surface* surface):
    parent_(parent), surface_(surface) {
  parent_->AddChild(surface);
  surface->SetParent(parent);
}

}  // namespace naive