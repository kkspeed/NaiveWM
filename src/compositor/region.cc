#include "region.h"
#include "base/logging.h"

namespace naive {

// static
Region Region::Empty() {
  base::geometry::Rect rect(0, 0, 0, 0);
  return Region(rect);
}

Region::Region(const base::geometry::Rect& rect) {
  pixman_region_ = std::shared_ptr<pixman_region32_t>(new pixman_region32_t(),
                                                      &pixman_region32_fini);
  pixman_region32_init(pixman_region_.get());
  pixman_region32_init_rect(pixman_region_.get(), rect.x(), rect.y(),
                            rect.width(), rect.height());
}

// TODO: Maybe region needs to be scoped region
Region::~Region() {
  TRACE();
  // pixman_region32_fini(&pixman_region_);
}

bool Region::is_empty() {
  return !pixman_region32_not_empty(pixman_region_.get());
}

void Region::Clear() {
  pixman_region32_clear(pixman_region_.get());
}

void Region::Union(Region& region) {
  pixman_region32_union(pixman_region_.get(), pixman_region_.get(),
                        region.pixman_region_.get());
}

void Region::Intersect(Region &region) {
  pixman_region32_intersect(pixman_region_.get(), pixman_region_.get(),
                            region.pixman_region_.get());
}

void Region::Subtract(Region& region) {
  pixman_region32_subtract(pixman_region_.get(), pixman_region_.get(),
                           region.pixman_region_.get());
}

Region Region::Clone() {
  Region result = Region::Empty();
  pixman_region32_copy(result.pixman_region_.get(), pixman_region_.get());
  return result;
}

Region Region::Translate(int32_t x, int32_t y) {
  TRACE("translating: %d %d", x, y);
  Region result = Clone();
  pixman_region32_translate(result.pixman_region_.get(), x, y);
  return result;
}

std::vector<base::geometry::Rect> Region::rectangles() {
  int32_t n;
  pixman_box32_t* boxes = pixman_region32_rectangles(pixman_region_.get(), &n);
  std::vector<base::geometry::Rect> result;

  for (int i = 0; i < n; i++) {
    TRACE("boxes: tl (%d %d), br(%d %d)", boxes[i].x1, boxes[i].y1, boxes[i].x2, boxes[i].y2);
    result.push_back(base::geometry::Rect(boxes[i].x1, boxes[i].y1,
                                          boxes[i].x2 - boxes[i].x1,
                                          boxes[i].y2 - boxes[i].y1));
  }
  return result;
}

}  // namespace naive
