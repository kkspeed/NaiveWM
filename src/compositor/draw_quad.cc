#include "compositor/draw_quad.h"

#include <wayland-server.h>

#include "compositor/buffer.h"

namespace naive {
namespace compositor {

DrawQuad::DrawQuad(Buffer* buffer)
    : width_(buffer->width()),
      height_(buffer->height()),
      data_(buffer->data()),
      format_(buffer->format()),
      stride_(buffer->stride()),
      has_data_(true) {}

DrawQuad::DrawQuad(int32_t width, int32_t height, void* data)
    : width_(width),
      height_(height),
      data_(data),
      format_(WL_SHM_FORMAT_ARGB8888),
      stride_(width_ * sizeof(uint32_t)),
      has_data_(true) {}

}  // namespace compositor
}  // namespace naive
