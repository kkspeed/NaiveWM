#include "compositor/draw_quad.h"

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

}  // namespace compositor
}  // namespace naive
