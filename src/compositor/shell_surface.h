#ifndef COMPOSITOR_SHELL_SURFACE_H
#define COMPOSITOR_SHELL_SURFACE_H

#include <cstdint>
#include <functional>

#include "base/geometry.h"
#include "compositor/surface.h"
#include "wm/window.h"

namespace naive {

namespace wm {
class Window;
}  // namespace wm;

class ShellSurface : SurfaceObserver {
 public:
  ShellSurface(Surface* surface);
  ~ShellSurface();

  void Move();
  void SetPosition(int32_t x, int32_t y);
  void SetGeometry(const base::geometry::Rect& rect);
  void SetVisibleRegion(const base::geometry::Rect& rect);
  void AcknowledgeConfigure(uint32_t serial);

  // SurfaceObserver overrides:
  void OnCommit(Surface* committed_surface) override;
  void OnSurfaceDestroyed(Surface* surface) override;

  void OnVisibilityChanged(bool visible) {
    visibility_changed_callback_(visible);
  }

  void set_close_callback(std::function<void()> callback) {
    close_callback_ = callback;
  }
  void set_destroy_callback(std::function<void()> callback) {
    destroy_callback_ = callback;
  }
  void set_configure_callback(
      std::function<uint32_t(int32_t, int32_t)> callback) {
    configure_callback_ = callback;
  }
  void set_ungrab_callback(std::function<void()> callback) {
    ungrab_callback_ = callback;
  }
  void set_activation_callback(std::function<void()> callback) {
    activation_callback_ = callback;
  }
  void set_visibility_changed_callback(std::function<void(bool)> callback) {
    visibility_changed_callback_ = callback;
  }

  void Configure(int32_t width, int32_t height);
  void Activate() { activation_callback_(); }
  void Close();
  void Ungrab() {
    TRACE("ungrabbing %p", this);
    ungrab_callback_();
    ungrab_callback_ = []() {};
  }

  wm::Window* window() { return window_; }

  void RecoverWindowState(ShellSurface* other);
  void CacheWindowState();
  int32_t CachedBufferScale() {
    if (!cached_window_state_)
      return -1;
    return cached_window_state_->buffer_scale_;
  }

  base::geometry::Rect* CachedBounds() {
    if (!cached_window_state_)
      return nullptr;
    return &cached_window_state_->geometry_;
  }

 private:
  struct ShellState {
    base::geometry::Rect geometry;
    base::geometry::Rect visible_region;
  };
  ShellState pending_state_, state_;

  std::function<uint32_t(int32_t, int32_t)> configure_callback_;
  std::function<void()> close_callback_;
  std::function<void()> destroy_callback_;
  std::function<void()> ungrab_callback_ = []() {};
  std::function<void()> activation_callback_ = []() {};
  std::function<void(bool)> visibility_changed_callback_ = [](bool) {};

  wm::Window* window_;
  Surface* surface_;
  bool in_configure_ = false;

  struct CachedWindowState {
    bool has_border_{false};
    bool is_popup_{false};
    bool is_transient_{false};
    wm::Window* parent_{nullptr};
    int32_t wm_x_ = 0, wm_y_ = 0;
    base::geometry::Rect geometry_;
    int32_t buffer_scale_ = 1;
  };

  // Used when window is destroyed before shell surface, so that
  // we may use this information to recover the window state.
  std::unique_ptr<CachedWindowState> cached_window_state_;
};

}  // namespace naive

#endif  // COMPOSITOR_SHELL_SURFACE_H
