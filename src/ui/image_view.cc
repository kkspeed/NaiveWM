#include "ui/image_view.h"

#include <cairomm/context.h>
#include <cairomm/surface.h>

#include "base/geometry.h"
#include "base/logging.h"
#include "wm/window_manager.h"

namespace naive {
namespace ui {

ImageView::ImageView(int32_t x,
                     int32_t y,
                     int32_t width,
                     int32_t height,
                     const std::string& path)
    : Widget(x, y, width, height) {
  TRACE("from image at %s, size: %d x %d", path.c_str(), surface_->get_width(),
        surface_->get_height());
  surface_ = Cairo::ImageSurface::create_from_png(path);
  context_->save();
  context_->set_source(surface_, 0.0, 0.0);
  context_->scale(width * 1.0f / surface_->get_width(),
                  height * 1.0f / surface_->get_height());
  context_->paint();
  context_->restore();
  AddDamage(base::geometry::Rect(0, 0, width, height));
  has_commit_ = true;
}

}  // namespace ui
}  // namespace naive
