#ifndef BASE_TIME_H_
#define BASE_TIME_H_

#include <cstdint>

namespace naive {
namespace base {

class Time {
 public:
  static uint32_t CurrentTimeMilliSeconds();
};

}  // namespace base
}  // namespace naive
#endif  // BASE_TIME_H_
