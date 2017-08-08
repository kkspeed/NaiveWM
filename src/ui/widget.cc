#include "ui/widget.h"

#include "wm/window.h"

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

}  // namespace ui
}  // namespace naive
