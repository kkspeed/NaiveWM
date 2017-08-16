#ifndef WINDOW_IMPL_H_
#define WINDOW_IMPL_H_

#include <memory>

#include "base/geometry.h"
#include "compositor/draw_quad.h"
#include "compositor/region.h"
#include "compositor/texture_delegate.h"

namespace naive {

namespace compositor {
class TextureDelegate;
}  // namespace compositor

namespace wm {

class WindowImpl {
 public:
  WindowImpl() = default;
  virtual ~WindowImpl() = default;

  // Called by window manager to notify the underline surface that
  // it no longer grabs mouse / keyboard input.
  virtual void SurfaceUngrab() {}

  // Called by window manager to notify the underline surface that
  // it should be closed.
  virtual void SurfaceClose() {}

  // Called by compositor to notify the surface that one frame is
  // being rendered.
  virtual void NotifyFrameRendered() = 0;

  // Add damage to surface area.
  virtual void AddDamage(const base::geometry::Rect& rect) = 0;

  // Whether this surface could be resized.
  virtual bool CanResize() { return false; }

  // Provide hint from the window manager to resize the surface.
  virtual void Configure(int32_t width, int32_t height) = 0;

  // Forces the surface to be fully redrawn.
  virtual void ForceCommit() = 0;

  // Whether the surface has newly committed content.
  virtual bool HasCommit() = 0;

  // Retrieves the damaged region of the underline surface.
  virtual Region DamagedRegion() = 0;

  // Gets the underline quad of the surface.
  virtual compositor::DrawQuad GetQuad() = 0;

  // Called by compositor that the commit is picked up.
  virtual void ClearCommit() = 0;

  // Called by compositor that the damage is picked up.
  virtual void ClearDamage() = 0;

  // Returns the scale of underlining surface implemenation.
  virtual int32_t GetScale() = 0;

  void CacheTexture(std::unique_ptr<compositor::TextureDelegate> texture);
  compositor::TextureDelegate* CachedTexture();

 private:
  std::unique_ptr<compositor::TextureDelegate> cached_texture_;
};

}  // namespace wm
}  // namespace naive

#endif  // WINDOW_IMPL_H_
