#ifndef _COMPOSITOR_SURFACE_H_
#define _COMPOSITOR_SURFACE_H_

#include <cstdint>

#include "base/geometry.h"
#include "compositor/region.h"

namespace naive {

class Buffer;

class Surface {
 public:
  void Attach(Buffer* buffer);
  void Damage(const base::geometry::Rect& rect);
  void SetOpaqueRegion(const Region& region);
  void SetInputRegion(const Region& region);
  void Commit();
  void SetBufferScale(int32_t scale);
};

}  // namespace naive

#endif  // _COMPOSITOR_SURFACE_H_
