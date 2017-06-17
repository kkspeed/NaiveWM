#ifndef _COMPOSITOR_SURFACE_H_
#define _COMPOSITOR_SURFACE_H_

#include <cstdint>
#include <memory>
#include <iostream>
#include <vector>
#include <wayland-server.h>

#include "base/geometry.h"
#include "base/logging.h"
#include "compositor/region.h"

namespace naive {

namespace wm {
class Window;
}  // namespace wm

class Buffer;

class SurfaceObserver {
 public:
  virtual void OnCommit() = 0;
};

class Surface {
 public:
  Surface();
  ~Surface() {
    LOG_ERROR << "surface dtor " << resource() << std::endl;
  }

  void Attach(Buffer* buffer);
  void Damage(const base::geometry::Rect& rect);
  void SetOpaqueRegion(const Region region);
  void SetInputRegion(const Region region);
  void Commit();
  void SetFrameCallback(std::function<void()>* callback);
  void SetBufferScale(int32_t scale){/* TODO: Implement this */};

  void AddSurfaceObserver(SurfaceObserver* observer) {
    observers_.push_back(observer);
  }

  void set_resource(wl_resource* resource) { resource_ = resource; }
  wl_resource* resource() { return resource_; }
  bool has_commit() { return has_commit_; }
  void clear_commit() { has_commit_ = false; }
  Buffer* committed_buffer() { return state_.buffer; }
  wm::Window* window() { return window_.get(); }

 private:
  struct SurfaceState {
    Region damaged_region = Region::Empty();
    Region opaque_region = Region::Empty();
    Region input_region = Region::Empty();
    Buffer* buffer = nullptr;
    std::function<void()>* frame_callback = nullptr;
  };

  SurfaceState pending_state_;
  SurfaceState state_;

  wl_resource* resource_;
  bool has_commit_ = false;
  std::vector<SurfaceObserver*> observers_;
  std::unique_ptr<wm::Window> window_;
};

}  // namespace naive

#endif  // _COMPOSITOR_SURFACE_H_
