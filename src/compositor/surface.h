#ifndef _COMPOSITOR_SURFACE_H_
#define _COMPOSITOR_SURFACE_H_

#include <cstdint>
#include <memory>

#include "base/geometry.h"
#include "compositor/region.h"
#include "wm/window.h"

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

  wm::Window* window() { return window_.get(); }
 private:
  std::unique_ptr<wm::Window> window_;
};

}  // namespace naive

#endif  // _COMPOSITOR_SURFACE_H_
