#ifndef _COMPOSITOR_REGION_H_
#define _COMPOSITOR_REGION_H_

#include <memory>
#include <pixman.h>

#include "base/geometry.h"

namespace naive {

class Region {
 public:
  static Region Empty();

  explicit Region(const base::geometry::Rect &rect);
  ~Region();

  void Union(Region &region);
  void Subtract(Region &region);

 private:
  std::shared_ptr<pixman_region32> pixman_region_;
};

}  // namespace naive

#endif  // _COMPOSITOR_REGION_H_
