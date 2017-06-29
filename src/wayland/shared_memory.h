#ifndef WAYLAND_SHARED_MEMORY_H_
#define WAYLAND_SHARED_MEMORY_H_

#include <sys/mman.h>
#include <cstdint>
#include <memory>

#include "base/logging.h"
#include "compositor/buffer.h"

namespace naive {

class Buffer;

namespace wayland {

class ShmPool {
 public:
  ShmPool(uint32_t size, void* data) : size_(size), data_(data) {
    TRACE("%p", this);
  }
  ~ShmPool() {
    TRACE("%p", this);
    munmap(data_, size_);
  }
  uint32_t size() { return size_; }
  void* data() { return data_; }
  void set_data(void* data, uint32_t size) {
    data_ = data;
    size_ = size;
  }

 private:
  uint32_t size_;
  void* data_;
};

class SharedMemory {
 public:
  explicit SharedMemory(int fd, uint32_t size);
  ~SharedMemory();

  void Resize(uint32_t size);
  std::unique_ptr<Buffer> CreateBuffer(int32_t width,
                                       int32_t height,
                                       int32_t format,
                                       int32_t offset,
                                       int32_t stride);

 private:
  std::shared_ptr<ShmPool> shm_data_;
};

}  // namespace wayland
}  // namespace naive

#endif  // WAYLAND_SHARED_MEMORY_H_
