#ifndef WAYLAND_SHARED_MEMORY_H_
#define WAYLAND_SHARED_MEMORY_H_

#include <cstdint>
#include <memory>

namespace naive {
namespace wayland {

class SharedMemory {
 public:
  explicit SharedMemory();
  std::unique_ptr<Buffer> CreateBuffer(int32_t width, int32_t height,
                                       int32_t format, int32_t offset,
                                       int32_t stride);
};

}  // namespace wayland
}  // namespace naive

#endif  // WAYLAND_SHARED_MEMORY_H_
