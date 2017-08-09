#ifndef UI_TEXT_VIEW_H_
#define UI_TEXT_VIEW_H_

#include <cstdint>
#include <string>
#include <type_traits>

#include "ui/widget.h"

namespace naive {
namespace ui {

enum class TextAlignment : uint32_t {
  LEFT = 1 << 1,
  CENTER_HORIZONTAL = 1 << 2,
  RIGHT = 1 << 3,
  TOP = 1 << 4,
  CENTER_VERTICAL = 1 << 5,
  BOTTOM = 1 << 6,
};

using T = std::underlying_type_t<TextAlignment>;

inline TextAlignment operator|(TextAlignment lhs, TextAlignment rhs) {
  return (TextAlignment)(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline TextAlignment operator&(TextAlignment lhs, TextAlignment rhs) {
  return (TextAlignment)(static_cast<T>(lhs) & static_cast<T>(rhs));
}

class TextView : public Widget {
 public:
  TextView(int32_t x, int32_t y, int32_t width, int32_t height);
  ~TextView() override = default;

  void SetTextAlignment(TextAlignment alignment);
  void SetBackgroundColor(uint32_t argb);
  void SetTextColor(uint32_t argb);
  void SetFont(const std::string& font);
  void SetTextSize(uint32_t size);
  void SetText(const std::string& text);
  const std::string& GetText() const;

  // Widget overrides.
  void Draw() override;

 protected:
  TextAlignment alignment_ =
      (TextAlignment::CENTER_HORIZONTAL | TextAlignment::CENTER_VERTICAL);
  uint32_t background_color_{0xFF000000};
  uint32_t text_color_{0xFFFFFFFF};
  std::string text_;
  std::string font_{"DejaVu Sans Mono"};
  uint32_t text_size_{10};
};

}  // namespace ui
}  // namespace naive

#endif  // UI_TEXT_VIEW_H_
