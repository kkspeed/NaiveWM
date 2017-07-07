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

namespace wm {
class Window;
}  // namespace wm

namespace compositor {

using CopyRequest = std::function<void(std::vector<uint8_t>, int32_t, int32_t)>;

class Compositor {
 public:
  static void InitializeCompoistor();
  static Compositor* Get();

  Compositor();

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

  void AddGlobalDamage(const base::geometry::Rect& rect);

 private:
  static Compositor* g_compositor;
  std::unique_ptr<wayland::DisplayMetrics> display_metrics_;
  bool draw_forced_ = true;
  std::unique_ptr<CopyRequest> copy_request_;
  Region global_damage_region_ = Region::Empty();
};

}  // namespace compositor
}  // namespace naive

#endif  // COMPOSITOR_COMPOSITOR_H_
