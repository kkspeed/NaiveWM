#include "base/logging.h"
#include "region.h"

namespace naive {

// static
Region Region::Empty() {
  base::geometry::Rect rect(0, 0, 0, 0);
  return Region(rect);
}

Region::Region(const base::geometry::Rect& rect) {
  pixman_region_ = std::shared_ptr<pixman_region32_t>(
      new pixman_region32_t(), &pixman_region32_fini);
  pixman_region32_init(pixman_region_.get());
  pixman_region32_init_rect(pixman_region_.get(), rect.x(), rect.y(),
                            rect.width(), rect.height());
}

// TODO: Maybe region needs to be scoped region
Region::~Region() {
  TRACE();
  // pixman_region32_fini(&pixman_region_);
}

void Region::Union(Region& region) {
  pixman_region32_union(pixman_region_.get(), pixman_region_.get(),
                        region.pixman_region_.get());
}

void Region::Subtract(Region& region) {
  pixman_region32_subtract(pixman_region_.get(), pixman_region_.get(),
                           region.pixman_region_.get());
}

}  // namespace naive
