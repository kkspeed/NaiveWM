#include "subsurface.h"

#include "wm/window.h"

namespace naive {

SubSurface::SubSurface(Surface* parent, Surface* surface)
    : parent_(parent), surface_(surface) {
  TRACE("%p, parent: %p, child: %p", this, parent, surface);
  // TODO: Observe child surface as well!
  parent_->window()->AddChild(surface->window());
  parent_->AddSurfaceObserver(this);
}

SubSurface::~SubSurface() {
  TRACE("%p %p", parent_, surface_);
  if (!parent_)
    return;

  if (surface_)
    parent_->window()->RemoveChild(surface_->window());
  parent_->RemoveSurfaceObserver(this);
}

void SubSurface::PlaceAbove(Surface* target) {
  pending_placement_.push_back(std::make_pair(true, target));
}

void SubSurface::PlaceBelow(Surface* target) {
  pending_placement_.push_back(std::make_pair(false, target));
}

void SubSurface::SetPosition(int32_t x, int32_t y) {
  pending_position_.x_ = x;
  pending_position_.y_ = y;
  position_dirty_ = true;
}

void SubSurface::SetCommitBehavior(bool sync) {
  is_synchronized_ = sync;
}

void SubSurface::OnCommit(Surface* committed_surface) {
  TRACE("parent: %p, surface: %p", parent_, surface_);
  while (!pending_placement_.empty()) {
    auto placement = pending_placement_.front();
    if (placement.first)
      parent_->window()->PlaceAbove(surface_->window(),
                                    placement.second->window());
    else
      parent_->window()->PlaceBelow(surface_->window(),
                                    placement.second->window());
    pending_placement_.pop_front();
  }

  // IF DIRTY POSITION
  if (position_dirty_) {
    position_ = pending_position_;
    surface_->window()->PushProperty(true, position_.x(), position_.y());
    position_dirty_ = false;
  }
  if (is_synchronized_) {
    LOG_ERROR << "Cascading commit" << std::endl;
    surface_->Commit();
  }
}

void SubSurface::OnSurfaceDestroyed(Surface* surface) {
  if (surface == surface_) {
    parent_->window()->RemoveChild(surface->window());
    surface_ = nullptr;
  }
  if (surface == parent_) {
    parent_ = nullptr;
  }
}

}  // namespace naive
