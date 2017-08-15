#include "buffer.h"
#include "surface.h"
#include "wayland/shared_memory.h"

namespace naive {

Buffer::Buffer(int32_t width,
               int32_t height,
               int32_t format,
               int32_t offset,
               int32_t stride,
               std::shared_ptr<wayland::ShmPool> pool)
    : width_(width),
      height_(height),
      format_(format),
      offset_(offset),
      stride_(stride),
      shm_pool_(pool) {}

Buffer::~Buffer() {
  TRACE("%p", this);
  if (owner_)
    owner_->NotifyBufferDestroyed(this);
}

void Buffer::SetOwningSurface(Surface* surface) {
  owner_ = surface;
}

void* Buffer::data() {
  uint8_t* p = static_cast<uint8_t*>(shm_pool_->data()) + offset_;
  return static_cast<void*>(p);
}

}  // namespace naive
