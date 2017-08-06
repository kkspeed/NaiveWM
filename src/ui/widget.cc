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
}

void Widget::GetTexture(std::vector<uint8_t>& buffer,
                        int32_t& width,
                        int32_t& height) {
  width = bounds_.width();
  height = bounds_.height();
  uint32_t bytes =
      static_cast<uint32_t>(surface_->get_stride() * surface_->get_height());
  buffer.resize(bytes);
  memcpy(buffer.data(), surface_->get_data(), bytes);
}

}  // namespace ui
}  // namespace naive
