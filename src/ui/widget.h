#ifndef UI_WIDGET_H_
#define UI_WIDGET_H_

#include <cstdint>
#include <vector>

#include <cairomm/context.h>
#include <cairomm/surface.h>

#include "base/geometry.h"
#include "compositor/region.h"

namespace naive {
namespace wm {
class Window;
}  // namespace window

namespace ui {

// Base class for server side widgets. It uses Cairo rendering.
class Widget {
 public:
  Widget(int32_t x, int32_t y, int32_t width, int32_t height);
  virtual ~Widget() = default;

  // When a frame is drawn.
  virtual void OnDrawFrame() {}

  // Called by compositor to get already texture drawn.
  virtual void GetTexture(std::vector<uint8_t>& buffer,
                          int32_t& width,
                          int32_t& height);

  virtual Region& GetDamagedRegion() { return damaged_region_; }

  // Get the bounds of this widget relative to its parent surface.
  virtual base::geometry::Rect& GetBounds() { return bounds_; }

 private:
  Region damaged_region_;
  base::geometry::Rect bounds_;
  Cairo::RefPtr<Cairo::Context> context_;
  Cairo::RefPtr<Cairo::ImageSurface> surface_;
  std::unique_ptr<wm::Window> window_;
};

}  // namespace ui
}  // namespace naive

#endif  // UI_WIDGET_H_
