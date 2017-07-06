#ifndef _COMPOSITOR_SURFACE_H_
#define _COMPOSITOR_SURFACE_H_

#include <wayland-server.h>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include "base/geometry.h"
#include "base/logging.h"
#include "compositor/region.h"
#include "compositor/texture_delegate.h"

namespace naive {

namespace wm {
class Window;
}  // namespace wm

namespace compositor {
class TextureDelegate;
}  // namespace compositor

class Buffer;
class Surface;

class SurfaceObserver {
 public:
  virtual void OnCommit() {}
  virtual void OnSurfaceDestroyed(Surface*) {}
};

class Surface {
 public:
  Surface();
  ~Surface();
  void Attach(Buffer* buffer);
  void Damage(const base::geometry::Rect& rect);
  void SetOpaqueRegion(const Region region);
  void SetInputRegion(const Region region);
  void Commit();
  void SetFrameCallback(std::function<void()>* callback);
  void SetBufferScale(int32_t scale){/* TODO: Implement this */};

  void AddSurfaceObserver(SurfaceObserver* observer) {
    TRACE("Add observer: %p to surface %p", observer, this);
    auto iter = std::find(observers_.begin(), observers_.end(), observer);
    if (iter == observers_.end())
      observers_.push_back(observer);
    for (auto* observer : observers_)
      TRACE("   now %p has observers: %p", this, observer);
  }
  void RemoveSurfaceObserver(SurfaceObserver* observer) {
    auto iter = std::find(observers_.begin(), observers_.end(), observer);
    if (iter != observers_.end()) {
      TRACE("Remove observer: %p from surface %p", observer, this);
      observers_.erase(iter);
    }
  }

  void RunSurfaceCallback() {
    if (state_.frame_callback) {
      (*state_.frame_callback)();
      state_.frame_callback = nullptr;
    }
  }

  void ForceDamage(base::geometry::Rect rect);
  Region damaged_regoin() { return state_.damaged_region; }
  void set_resource(wl_resource* resource) { resource_ = resource; }
  wl_resource* resource() { return resource_; }
  bool has_commit() { return has_commit_; }
  void force_commit() { has_commit_ = true; }
  void clear_commit() { has_commit_ = false; }
  void clear_damage() { state_.damaged_region = Region::Empty(); }
  Buffer* committed_buffer() { return state_.buffer; }
  wm::Window* window() {
    assert(window_);
    return window_.get();
  }
  std::function<void()>* frame_callback() { return state_.frame_callback; }

  compositor::TextureDelegate* cached_texture() {
    return cached_texture_ ? cached_texture_.get() : nullptr;
  }
  void cache_texture(std::unique_ptr<compositor::TextureDelegate> texture) {
    cached_texture_ = std::move(texture);
  }

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

  std::unique_ptr<compositor::TextureDelegate> cached_texture_;
};

}  // namespace naive

#endif  // _COMPOSITOR_SURFACE_H_
