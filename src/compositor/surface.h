#ifndef _COMPOSITOR_SURFACE_H_
#define _COMPOSITOR_SURFACE_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "base/geometry.h"
#include "compositor/region.h"
#include "wm/window.h"

namespace naive {

class Buffer;

class SurfaceObserver {
 public:
  virtual void OnCommit() = 0;
};

class Surface {
 public:
  Surface();

  void Attach(Buffer* buffer);
  void Damage(const base::geometry::Rect& rect);
  void SetOpaqueRegion(const Region& region);
  void SetInputRegion(const Region& region);
  void Commit();
  void SetBufferScale(int32_t scale) { /* TODO: Implement this */ };

  wm::Window* window() { return window_.get(); }

 private:
  struct SurfaceState {
    Region damaged_region = Region::Empty();
    Region opaque_region = Region::Empty();
    Region input_region = Region::Empty();
    Buffer* buffer = nullptr;
  };

  SurfaceState pending_state_;
  SurfaceState state_;

  std::unique_ptr<wm::Window> window_;
};

}  // namespace naive

#endif  // _COMPOSITOR_SURFACE_H_
