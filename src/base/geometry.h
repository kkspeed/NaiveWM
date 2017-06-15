#ifndef _BASE_GEOMETRY_H_
#define _BASE_GEOMETRY_H_

#include <cstdint>
#include <cmath>

namespace base {
namespace geometry {

template<typename T>
class Point {
 public:
  Point(T x, T y) : x_(x), y_(y) {}
  T x() { return x_; }
  T y() { return y_; }
  T manhattan_distance(const Point<T> &other) {
    return static_cast<T>(fabs(static_cast<float>(other.x_ - x_))
        + fabs(static_cast<float>(other.y_ - y_)));
  }
 private:
  T x_, y_;
};

using FloatPoint = Point<float>;
using IntPoint = Point<int>;

class Rect {
 public:
  Rect(int32_t x, int32_t y, int32_t width, int32_t height)
      : x_(x), y_(y), width_(width), height_(height) {}
  ~Rect() = default;

  bool ContainsPoint(int32_t x, int32_t y) {
    return x_ <= x && x <= x_ + width_ && y_ <= y && y <= y_ + height_;
  }

  int32_t x() const { return x_; }
  int32_t y() const { return y_; }
  int32_t width() const { return width_; }
  int32_t height() const { return height_; }

  int32_t x_, y_, width_, height_;
};

}  // namespace geometry
}  // namespace base

#endif  // _BASE_GEOMETRY_H_
