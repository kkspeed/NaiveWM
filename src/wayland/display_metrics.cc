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

  int32_t pixel_per_mm = round(sqrt(w * w + h * h) / sqrt(pw * pw + ph * ph));
  scale = pixel_per_mm / k1xPixelPerMm;

  width_dp = width_pixels / scale;
  height_dp = height_pixels / scale;

  TRACE("scale: %d, pixel_per_mm: %d", scale, pixel_per_mm);
}

}  // namespace wayland
}  // namespace naive