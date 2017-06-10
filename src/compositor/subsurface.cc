#include "subsurface.h"

namespace naive {

SubSurface::SubSurface(Surface* parent, Surface* surface):
    parent_(parent), surface_(surface) {
  parent_->window()->AddChild(surface->window());
}

void SubSurface::PlaceAbove(Surface* target) {
  parent_->window()->PlaceAbove(surface_->window(), target->window());
}

void SubSurface::PlaceBelow(Surface* target) {
  parent_->window()->PlaceBelow(surface_->window(), target->window());
}

void SubSurface::SetPosition(int32_t x, int32_t y) {
  surface_->window()->SetPosition(x, y);
}

void SubSurface::SetCommitBehavior(bool sync) {
  is_synchronized_ = sync;
}

void SubSurface::OnCommit() {
  if (is_synchronized_)
    surface_->Commit();
}

}  // namespace naive