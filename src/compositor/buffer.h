#ifndef _COMPOSITOR_BUFFER_H_
#define _COMPOSITOR_BUFFER_H_

#include <functional>
#include <memory>
#include <vector>

#include "base/logging.h"

namespace naive {

namespace wayland {
class ShmPool;
class SharedMemory;
}  // namespace wayland

class Surface;

class Buffer {
 public:
  Buffer(int32_t width,
         int32_t height,
         int32_t format,
         int32_t offset,
         int32_t stride,
         std::shared_ptr<wayland::ShmPool> pool);
  ~Buffer() { TRACE("%p", this); }
  void SetOwningSurface(Surface* surface);
  void* data();

  void set_release_callback(std::function<void()> callback) {
    buffer_release_callback_ = callback;
  }

  void Release() { buffer_release_callback_(); }

  void CopyLocal() {
    buffer_data_.resize(width_ * height_ * sizeof(uint32_t));
    memcpy(buffer_data_.data(), data(), width_ * height_ * sizeof(uint32_t));
  }

  void* local_data() { return buffer_data_.data(); }

  int32_t width() { return width_; }
  int32_t height() { return height_; }
  int32_t format() { return format_; }
  int32_t stride() { return stride_; }
  int32_t offset() { return offset_; }

 private:
  int32_t width_, height_, format_, offset_, stride_;
  std::shared_ptr<wayland::ShmPool> shm_pool_;
  std::vector<uint8_t> buffer_data_;
  Surface* owner_;
  std::function<void()> buffer_release_callback_;
};

}  // namespace naive

#endif  // _COMPOSITOR_BUFFER_H_
