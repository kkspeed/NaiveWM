#ifndef COMPOSITOR_COMPOSITOR_H_
#define COMPOSITOR_COMPOSITOR_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "base/geometry.h"
#include "compositor/region.h"
#include "wayland/display_metrics.h"

namespace naive {
namespace backend {
class EglContext;
class Backend;
}  // namespace backend

namespace wm {
class Window;
}  // namespace wm

namespace compositor {

class GlRenderer;

using CopyRequest = std::function<void(std::vector<uint8_t>, int32_t, int32_t)>;

class Compositor {
 public:
  static void InitializeCompoistor(backend::Backend* backend);
  static Compositor* Get();

  Compositor(backend::Backend* backend);

  wayland::DisplayMetrics* GetDisplayMetrics();

  void CopyScreen(std::unique_ptr<CopyRequest> request) {
    copy_request_ = std::move(request);
  }
  void Draw();
  void DrawPointer();
  void FillRect(base::geometry::Rect rect, float r, float g, float b);
  void DrawWindowBorder(wm::Window* window);
  void DrawWindowRecursive(wm::Window* window,
                           int32_t start_x,
                           int32_t start_y);

  void AddGlobalDamage(const base::geometry::Rect& rect, wm::Window* window);

 private:
  static Compositor* g_compositor;
  backend::Backend* backend_;
  backend::EglContext* egl_;
  wayland::DisplayMetrics* display_metrics_;
  bool draw_forced_ = true;
  std::unique_ptr<CopyRequest> copy_request_;
  Region global_damage_region_ = Region::Empty();
  std::unique_ptr<GlRenderer> renderer_;
};

}  // namespace compositor
}  // namespace naive

#endif  // COMPOSITOR_COMPOSITOR_H_
