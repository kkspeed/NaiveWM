#include "shared_memory.h"

#include <wayland-server.h>

#include <sys/mman.h>

namespace naive {
namespace wayland {

SharedMemory::SharedMemory(int fd, uint32_t size) {
  void* data = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
  shm_data_ = std::make_shared<ShmPool>(size, data);
}

std::unique_ptr<Buffer> SharedMemory::CreateBuffer(int32_t width,
                                                   int32_t height,
                                                   int32_t format,
                                                   int32_t offset,
                                                   int32_t stride) {
  return std::make_unique<Buffer>(width,
                                  height,
                                  format,
                                  offset,
                                  stride,
                                  shm_data_);
}

}  // namespace wayland
}  // naive