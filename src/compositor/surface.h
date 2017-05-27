#ifndef _COMPOSITOR_SURFACE_H_
#define _COMPOSITOR_SURFACE_H_

#include "base/geometry.h"

namespace naive {

class Buffer;

class Surface {
 public:
  void Attach(Buffer* buffer);
  void Damage(const base::geometry::Rect& rect);
};

}  // namespace naive

#endif  // _COMPOSITOR_SURFACE_H_
