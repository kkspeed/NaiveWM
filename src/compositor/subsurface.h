#ifndef COMPOSITOR_SUBSURFACE_H_
#define COMPOSITOR_SUBSURFACE_H_

#include <cstdint>
#include "compositor/surface.h"

namespace naive {

class SubSurface {
 public:
  SubSurface(Surface* parent, Surface* surface);
  void SetPosition(int32_t x, int32_t y);
  void PlaceAbove(Surface* target);
  void PlaceBelow(Surface* target);
  void SetCommitBehavior(bool sync);
};

}  // namespace naive

#endif  // COMPOSITOR_SUBSURFACE_H_
