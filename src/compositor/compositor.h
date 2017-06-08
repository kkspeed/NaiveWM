#ifndef COMPOSITOR_COMPOSITOR_H_
#define COMPOSITOR_COMPOSITOR_H_

namespace naive {
namespace compositor {

class Compositor {
 public:
  static Compositor* Get();
  bool Draw();
};

}  // namespace compositor
}  // namespace naive

#endif  // COMPOSITOR_COMPOSITOR_H_
