#ifndef UI_WIDGET_H_
#define UI_WIDGET_H_

#include <cstdint>
#include <vector>

#include <cairomm/context.h>
#include <cairomm/surface.h>

#include "base/geometry.h"
#include "compositor/region.h"
#include "wm/window.h"

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
  virtual void OnDrawFrame();

  // When the view needs to be drawn.
  virtual void Draw() {}

  void* GetTexture(int32_t& width, int32_t& height);

  Region& GetDamagedRegion() { return damaged_region_; }
  void AddDamage(const base::geometry::Rect& rect);

  // Get the bounds of this widget relative to its parent surface.
  base::geometry::Rect& GetBounds() { return bounds_; }

  wm::Window* window() { return window_.get(); }
  bool has_commit() { return has_commit_; }
  void force_commit() { has_commit_ = true; }
  void clear_commit() { has_commit_ = false; }
  void clear_damage() { damaged_region_ = Region::Empty(); }

  // Marks the view as dirty and needs to be drawn.
  void Invalidate();

  void AddChild(Widget* widget);
  void RemoveChild(Widget* widget);

 protected:
  bool has_commit_{false};
  bool view_dirty_{false};
  Region damaged_region_;
  base::geometry::Rect bounds_;
  Cairo::RefPtr<Cairo::Context> context_;
  Cairo::RefPtr<Cairo::ImageSurface> surface_;
  std::unique_ptr<wm::Window> window_;
};

}  // namespace ui
}  // namespace naive

#endif  // UI_WIDGET_H_
