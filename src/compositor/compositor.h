#ifndef COMPOSITOR_COMPOSITOR_H_
#define COMPOSITOR_COMPOSITOR_H_

namespace naive {
namespace compositor {

class Compositor {
 public:
  static void InitializeCompoistor();
  static Compositor* Get();

  Compositor();
  bool NeedToDraw();
  void Draw();

  void DrawPointer();

 private:
  static Compositor* g_compositor;
  bool draw_forced_ = true;
};

}  // namespace compositor
}  // namespace naive

#endif  // COMPOSITOR_COMPOSITOR_H_
