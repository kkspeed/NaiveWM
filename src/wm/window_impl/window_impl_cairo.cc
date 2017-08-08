#include "wm/window_impl/window_impl_cairo.h"

#include "base/logging.h"
#include "ui/widget.h"

namespace naive {
namespace wm {

WindowImplCairo::WindowImplCairo(ui::Widget* widget) : widget_(widget) {}

void WindowImplCairo::NotifyFrameRendered() {
  widget_->OnDrawFrame();
}

void WindowImplCairo::AddDamage(const base::geometry::Rect& rect) {
  widget_->AddDamage(rect);
}

void WindowImplCairo::ForceCommit() {
  widget_->force_commit();
}

bool WindowImplCairo::HasCommit() {
  TRACE();
  return widget_->has_commit();
}

Region WindowImplCairo::DamagedRegion() {
  return widget_->GetDamagedRegion();
}

compositor::DrawQuad WindowImplCairo::GetQuad() {
  int32_t width, height;
  void* data = widget_->GetTexture(width, height);
  if (!data)
    return compositor::DrawQuad();
  return compositor::DrawQuad(width, height, data);
}

void WindowImplCairo::ClearCommit() {
  widget_->clear_commit();
}

void WindowImplCairo::ClearDamage() {
  widget_->clear_damage();
}

}  // namespace wm
}  // namespace naive
