#ifndef _BASE_GEOMETRY_H_
#define _BASE_GEOMETRY_H_

#include <cstdint>

namespace base {
namespace geometry {

class Rect {
 public:
  Rect(int32_t x, int32_t y, int32_t width, int32_t height);
};

}  // namespace geometry
}  // namespace base

#endif  // _BASE_GEOMETRY_H_
