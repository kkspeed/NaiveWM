#include "base/time.h"

#include <chrono>

namespace naive {
namespace base {

// static
uint32_t Time::CurrentTimeMilliSeconds() {
  return static_cast<uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

}  // namespace base
}  // namespace naive
