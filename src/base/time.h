#ifndef BASE_TIME_H_
#define BASE_TIME_H_

#include <cstdint>
#include <string>

namespace naive {
namespace base {

class Time {
 public:
  static uint32_t CurrentTimeMilliSeconds();
  static std::string GetTime(const char* format);
};

}  // namespace base
}  // namespace naive
#endif  // BASE_TIME_H_
