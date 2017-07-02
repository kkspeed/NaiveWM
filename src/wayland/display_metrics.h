#ifndef WAYLAND_DISPLAY_METRICS_H_
#define WAYLAND_DISPLAY_METRICS_H_

#include <cstdint>

namespace naive {
namespace wayland {

struct DisplayMetrics {
  DisplayMetrics(int32_t w, int32_t h, int32_t pw, int32_t ph);

  // Monitor's width and height pixels.
  int32_t width_pixels, height_pixels;
  // Monitor's width and height in dp. width_dp = width_pixels / scale.
  int32_t width_dp, height_dp;
  int32_t scale;

  // Monitor width and height in millimeter.
  int32_t physical_width, physical_height;
};

}  // namespace wayland
}  // namespace naive

#endif  // WAYLAND_DISPLAY_METRICS_H_
