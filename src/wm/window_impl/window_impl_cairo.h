#ifndef WM_WINDOW_IMPL_WINDOW_IMPL_CAIRO_H_
#define WM_WINDOW_IMPL_WINDOW_IMPL_CAIRO_H_

#include "wm/window_impl.h"

#include "base/geometry.h"
#include "compositor/draw_quad.h"
#include "compositor/region.h"

namespace naive {
namespace ui {
class Widget;
}  // namespace ui

namespace wm {

class WindowImplCairo : public WindowImpl {
 public:
  WindowImplCairo(ui::Widget* widget);

  // WindowImpl overrides.
  void NotifyFrameRendered() override;
  void AddDamage(const base::geometry::Rect& rect) override;
  void Configure(int32_t width, int32_t height) override {}
  void TakeFocus() override {}
  void ForceCommit() override;
  bool HasCommit() override;
  Region DamagedRegion() override;
  compositor::DrawQuad GetQuad() override;
  void ClearCommit() override;
  void ClearDamage() override;
  int32_t GetScale() override;

 private:
  ui::Widget* widget_;
};

}  // namespace wm
}  // namespace naive

#endif  // WM_WINDOW_IMPL_WINDOW_IMPL_CAIRO_H_
