#include "wayland/display_metrics.h"

#include <cmath>

#include "base/logging.h"

namespace naive {
namespace wayland {

constexpr int32_t k1xPixelPerMm = 4;

DisplayMetrics::DisplayMetrics(int32_t w, int32_t h, int32_t pw, int32_t ph) {
  TRACE("px: %d %d, phy: %d %d", w, h, pw, ph);
  width_pixels = w;
  height_pixels = h;
  physical_width = pw;
  physical_height = ph;

  scale = round(sqrt(w * w + h * h) / sqrt(pw * pw + ph * ph) / k1xPixelPerMm);

  // Minimum of 1x scale is expected.
  if (scale < 1)
    scale = 1;

  width_dp = width_pixels / scale;
  height_dp = height_pixels / scale;
}

}  // namespace wayland
}  // namespace naive
