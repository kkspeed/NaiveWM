#ifndef _BASE_GEOMETRY_H_
#define _BASE_GEOMETRY_H_

#include <cstdint>

namespace base {
namespace geometry {

class Rect {
 public:
  Rect(int32_t x, int32_t y, int32_t width, int32_t height);
  ~Rect() = default;

  int32_t x() const { return x_; }
  int32_t y() const { return y_; }
  int32_t width() const { return width_; }
  int32_t height() const { return height_; }

  int32_t x_, y_, width_, height_;
};

}  // namespace geometry
}  // namespace base

#endif  // _BASE_GEOMETRY_H_
