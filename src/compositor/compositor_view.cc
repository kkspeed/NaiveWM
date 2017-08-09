#include "compositor/compositor_view.h"

#include "compositor/surface.h"
#include "wm/window_impl.h"

namespace naive {
namespace compositor {

namespace {

void CollectViewsAt(int32_t x_offset,
                    int32_t y_offset,
                    wm::Window* window,
                    CompositorViewList& list) {
  auto rect = window->geometry();
  int32_t x = x_offset;
  int32_t y = y_offset;
  TRACE("window: %p, global_x: %d, global_y: %d", window, x, y);
  auto view = std::make_unique<CompositorView>(window, x, y);
  list.push_back(std::move(view));
  for (auto* child : window->children())
    CollectViewsAt(x + rect.x(), y + rect.y(), child, list);
}

}  // namespace

// static
CompositorViewList CompositorView::BuildCompositorViewHierarchyRecursive(
    wm::Window* window) {
  CompositorViewList result;
  CollectViewsAt(window->wm_x(), window->wm_y(), window, result);
  return result;
}

CompositorView::CompositorView(wm::Window* window,
                               int32_t x_offset,
                               int32_t y_offset)
    : window_(window),
      damaged_region_(window->window_impl()->DamagedRegion().Clone()),
      global_bounds_(window->geometry()),
      global_region_(Region(window->geometry())) {
  damaged_region_.Intersect(window->GetToDrawRegion());
  damaged_region_.TranslateInPlace(x_offset + global_bounds_.x(),
                                   y_offset + global_bounds_.y());
  global_bounds_.x_ += x_offset;
  global_bounds_.y_ += y_offset;
  global_region_.TranslateInPlace(x_offset, y_offset);
  if (!window->parent()) {
    auto rect = base::geometry::Rect(
        global_bounds_.x() + 2, global_bounds_.y() + 2,
        global_bounds_.width() - 4, global_bounds_.height() - 4);
    Region inner(rect);
    border_region_ = global_region_.Clone();
    border_region_.Subtract(inner);
  }
}

}  // namespace compositor
}  // namespace naive
