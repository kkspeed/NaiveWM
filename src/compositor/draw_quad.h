#ifndef COMPOSITOR_DRAW_QUAD_H_
#define COMPOSITOR_DRAW_QUAD_H_

#include <cstdint>

namespace naive {

class Buffer;

namespace compositor {

class DrawQuad {
 public:
  DrawQuad() = default;
  explicit DrawQuad(Buffer* buffer);
  explicit DrawQuad(int32_t width, int32_t height, void* data);

  bool has_data() { return has_data_; }
  int32_t width() { return width_; }
  int32_t height() { return height_; }
  int32_t format() { return format_; }
  int32_t stride() { return stride_; }
  void* data() { return data_; }

 private:
  bool has_data_{false};
  int32_t format_, width_, height_, stride_;
  void* data_;
};

}  // namespace compositor
}  // namespace naive

#endif  // COMPOSITOR_DRAW_QUAD_H_
