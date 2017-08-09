#include "ui/widget.h"

#include "base/logging.h"

namespace naive {
namespace ui {

Widget::Widget(int32_t x, int32_t y, int32_t width, int32_t height)
    : bounds_(base::geometry::Rect(x, y, width, height)),
      damaged_region_(Region::Empty()),
      window_(std::make_unique<wm::Window>(this)) {
  surface_ = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, width, height);
  context_ = Cairo::Context::create(surface_);
  window_->PushProperty(base::geometry::Rect(x, y, width, height),
                        base::geometry::Rect());
}

void* Widget::GetTexture(int32_t& width, int32_t& height) {
  width = bounds_.width();
  height = bounds_.height();
  return surface_->get_data();
}

void Widget::OnDrawFrame() {
  if (view_dirty_) {
    Draw();
    view_dirty_ = false;
  }
}

void Widget::Invalidate() {
  has_commit_ = true;
  AddDamage(base::geometry::Rect(0, 0, bounds_.width(), bounds_.height()));
  view_dirty_ = true;
}

void Widget::AddDamage(const base::geometry::Rect& rect) {
  TRACE("%s", base::geometry::Rect(rect).ToString().c_str());
  damaged_region_.Union(rect);
}

void Widget::AddChild(Widget* widget) {
  window_->AddChild(widget->window());
}

void Widget::RemoveChild(Widget* widget) {
  window_->RemoveChild(widget->window());
}

}  // namespace ui
}  // namespace naive
