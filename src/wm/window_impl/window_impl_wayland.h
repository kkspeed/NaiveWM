#ifndef WINDOW_IMPL_WAYLAND_H_
#define WINDOW_IMPL_WAYLAND_H_

#include "compositor/draw_quad.h"
#include "compositor/region.h"
#include "wm/window_impl.h"

namespace naive {

class ShellSurface;
class Surface;

namespace wm {

class WindowImplWayland : public WindowImpl {
 public:
  explicit WindowImplWayland(Surface* surface);
  ~WindowImplWayland();

  void set_shell_surface(ShellSurface* shell_surface);

  // WindowImpl overrides
  void SurfaceUngrab() override;
  void SurfaceClose() override;
  void NotifyFrameRendered() override;
  void AddDamage(const base::geometry::Rect& rect) override;
  bool CanResize() override;
  void Configure(int32_t width, int32_t height) override;
  void TakeFocus() override;
  void ForceCommit() override;
  bool HasCommit() override;
  Region DamagedRegion() override;
  compositor::DrawQuad GetQuad() override;
  void ClearCommit() override;
  void ClearDamage() override;
  int32_t GetScale() override;

 private:
  Surface* surface_{nullptr};
  ShellSurface* shell_surface_{nullptr};
};

}  // namespace wm
}  // namespace naive

#endif  // WINDOW_IMPL_WAYLAND_H_
