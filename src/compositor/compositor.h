#ifndef COMPOSITOR_COMPOSITOR_H_
#define COMPOSITOR_COMPOSITOR_H_

#include <cstdint>

namespace naive {

namespace wm {
class Window;
}  // namespace wm

namespace compositor {

class Compositor {
 public:
  static void InitializeCompoistor();
  static Compositor* Get();

  Compositor();
  void Draw();
  void DrawPointer();
  void DrawWindowBorder(wm::Window* window);
  void DrawWindowRecursive(wm::Window* window,
                           int32_t start_x,
                           int32_t start_y);

 private:
  static Compositor* g_compositor;
  bool draw_forced_ = true;
};

}  // namespace compositor
}  // namespace naive

#endif  // COMPOSITOR_COMPOSITOR_H_
