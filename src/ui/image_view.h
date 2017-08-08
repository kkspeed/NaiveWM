#ifndef UI_IMAGE_VIEW_H_
#define UI_IMAGE_VIEW_H_

#include "ui/widget.h"

#include <string>

namespace naive {
namespace ui {

class ImageView : public Widget {
public:
    ImageView(int32_t x, int32_t y, int32_t width, int32_t height,
              const std::string& path);
};

}  // namespace ui
}  // namespace naive

#endif  // UI_IMAGE_VIEW_H_
