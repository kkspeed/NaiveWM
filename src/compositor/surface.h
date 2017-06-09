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

  void SetParent(Surface* parent) { parent_ = parent; };

  void AddChild(Surface* surface);
  void RemoveChild(Surface* surface);

 private:
  struct SurfaceState {
    Region damaged_region = Region::Empty();
    Region opaque_region = Region::Empty();
    Region input_region = Region::Empty();
    Buffer* buffer = nullptr;
  };

  SurfaceState pending_state_;
  SurfaceState state_;

  Surface* parent_;
  std::vector<Surface*> children_;
  std::unique_ptr<wm::Window> window_;
};

}  // namespace naive

#endif  // _COMPOSITOR_SURFACE_H_
