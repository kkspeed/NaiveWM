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

// static
std::string Time::GetTime(const char* format) {
  char buffer[256];
  auto t = std::time(nullptr);
  auto* tm = std::localtime(&t);
  std::strftime(buffer, 256, format, tm);
  return std::string(buffer);
}

}  // namespace base
}  // namespace naive
