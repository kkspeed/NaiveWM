#include "ui/text_view.h"

#include <cairomm/context.h>
#include <cairomm/surface.h>

#include "base/logging.h"

namespace naive {
namespace ui {

namespace {

void ArgbToDouble(uint32_t argb, double& a, double& r, double& g, double& b) {
  a = ((argb >> 24) & 0xFF) / 255.0;
  r = ((argb >> 16) & 0xFF) / 255.0;
  g = ((argb >> 8) & 0xFF) / 255.0;
  b = (argb & 0xFF) / 255.0;
}

}  // namespace

TextView::TextView(int32_t x, int32_t y, int32_t width, int32_t height)
    : Widget(x, y, width, height) {}

void TextView::SetTextAlignment(TextAlignment alignment) {
  alignment_ = alignment;
  Invalidate();
}

void TextView::SetBackgroundColor(uint32_t argb) {
  if (argb != background_color_) {
    background_color_ = argb;
    Invalidate();
  }
}

void TextView::SetTextColor(uint32_t argb) {
  if (text_color_ != argb) {
    text_color_ = argb;
    Invalidate();
  }
}

void TextView::SetFont(const std::string& font) {
  font_ = font;
  Invalidate();
}

void TextView::SetTextSize(uint32_t size) {
  if (text_size_ != size) {
    text_size_ = size;
    Invalidate();
  }
}

void TextView::SetText(const std::string& text) {
  text_ = text;
  Invalidate();
}

const std::string& TextView::GetText() const {
  return text_;
}

void TextView::Draw() {
  double bgr, bgg, bgb, bga;
  ArgbToDouble(background_color_, bga, bgr, bgg, bgb);

  if (text_.empty())
    return;

  double fgr, fgg, fgb, fga;
  ArgbToDouble(text_color_, fga, fgr, fgg, fgb);

  double center_x = bounds_.width() / 2.0;
  double center_y = bounds_.height() / 2.0;

  double text_x, text_y;
  context_->save();
  context_->set_source_rgba(bgr, bgg, bgb, bga);
  context_->rectangle(0, 0, bounds_.width(), bounds_.height());
  context_->fill();

  context_->set_source_rgba(fgr, fgg, fgb, fga);
  context_->set_font_size(text_size_);
  context_->select_font_face(font_, Cairo::FONT_SLANT_NORMAL,
                             Cairo::FONT_WEIGHT_NORMAL);
  Cairo::TextExtents extents;
  context_->get_text_extents(text_, extents);

  if (static_cast<bool>(alignment_ & TextAlignment::LEFT))
    text_x = 0;
  if (static_cast<bool>(alignment_ & TextAlignment::RIGHT))
    text_x = bounds_.width() - extents.width - extents.x_bearing;
  if (static_cast<bool>(alignment_ & TextAlignment::CENTER_HORIZONTAL))
    text_x = center_x - extents.width / 2.0 - extents.x_bearing;
  if (static_cast<bool>(alignment_ & TextAlignment::TOP))
    text_y = 0;
  if (static_cast<bool>(alignment_ & TextAlignment::BOTTOM))
    text_y = bounds_.height() - extents.height - extents.y_bearing;
  if (static_cast<bool>(alignment_ & TextAlignment::CENTER_VERTICAL))
    text_y = center_y - extents.height / 2.0 - extents.y_bearing;
  context_->move_to(text_x, text_y);
  context_->show_text(text_);
  context_->restore();
}

}  // namespace ui
}  // namespace naive
