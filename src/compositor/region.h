#ifndef _COMPOSITOR_REGION_H_
#define _COMPOSITOR_REGION_H_

#include <pixman.h>

#include "base/geometry.h"

namespace naive {

class Region {
 public:
  static Region Empty();

  explicit Region(const base::geometry::Rect& rect);
  ~Region();

  void Union(const Region& region);
  void Subtract(const Region& region);
 private:
  pixman_region32_t pixman_region_;
};

}  // namespace naive

#endif  // _COMPOSITOR_REGION_H_
