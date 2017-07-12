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

  int32_t width() { return width_; }
  int32_t height() { return height_; }
  int32_t format() { return format_; }
  int32_t stride() { return stride_; }
  int32_t offset() { return offset_; }

 private:
  int32_t width_, height_, format_, offset_, stride_;
  std::shared_ptr<wayland::ShmPool> shm_pool_;
  Surface* owner_;
  std::function<void()> buffer_release_callback_;
};

}  // namespace naive

#endif  // _COMPOSITOR_BUFFER_H_
