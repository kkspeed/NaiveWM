#ifndef COMPOSITOR_COMPOSITOR_VIEW_H_
#define COMPOSITOR_COMPOSITOR_VIEW_H_

#include <memory>
#include <vector>

#include "base/geometry.h"
#include "compositor/region.h"
#include "wm/window.h"

namespace naive {
namespace compositor {

class CompositorView;
using CompositorViewList = std::vector<std::unique_ptr<CompositorView>>;

// A compositor view is a representation of windows in global coordinates.
class CompositorView {
 public:
  explicit CompositorView(wm::Window* window,
                          int32_t x_offset,
                          int32_t y_offset);

  static CompositorViewList BuildCompositorViewHierarchyRecursive(
      wm::Window* window);

  base::geometry::Rect& global_bounds() { return global_bounds_; }
  Region& global_region() { return global_region_; }
  Region& border_region() { return border_region_; }
  Region& damaged_region() { return damaged_region_; }
  wm::Window* window() { return window_; }

 private:
  wm::Window* window_;
  base::geometry::Rect global_bounds_;
  Region global_region_;
  Region damaged_region_;
  Region border_region_ = Region::Empty();
};

}  // namespace compositor
}  // namespace naive

#endif  // COMPOSITOR_COMPOSITOR_VIEW_H_
