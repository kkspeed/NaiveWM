#include "region.h"

namespace naive {

// static
Region Region::Empty() {
  base::geometry::Rect rect(0, 0, 0, 0);
  return Region(rect);
}

Region::Region(const base::geometry::Rect& rect) {
  pixman_region32_init(&pixman_region_);
  pixman_region32_init_rect(&pixman_region_, rect.x(), rect.y(), rect.width(),
                            rect.height());
}

Region::~Region() {
  pixman_region32_fini(&pixman_region_);
}

void Region::Union(Region& region) {
  pixman_region32_union(&pixman_region_, &pixman_region_,
                        &region.pixman_region_);
}

void Region::Subtract(Region& region) {
  pixman_region32_subtract(&pixman_region_, &pixman_region_,
                           &region.pixman_region_);
}

}  // namespace naive
