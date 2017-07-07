#ifndef _COMPOSITOR_REGION_H_
#define _COMPOSITOR_REGION_H_

#include <pixman.h>
#include <memory>
#include <vector>

#include "base/geometry.h"

namespace naive {

class Region {
 public:
  static Region Empty();

  explicit Region(const base::geometry::Rect& rect);
  ~Region();

  bool is_empty();
  void Clear();
  void Union(Region& region);
  void Union(const base::geometry::Rect& rect);
  void Subtract(Region& region);
  void Intersect(Region& region);
  void Intersect(const base::geometry::Rect& rect);
  Region Translate(int32_t x, int32_t y);
  void TranslateInPlace(int32_t x, int32_t y);
  Region Clone();

  std::vector<base::geometry::Rect> rectangles();

 private:
  std::shared_ptr<pixman_region32> pixman_region_;
};

}  // namespace naive

#endif  // _COMPOSITOR_REGION_H_
