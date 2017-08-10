#include "wm/manage/panel.h"

#include <ctime>
#include <sstream>
#include <string>

#include "base/logging.h"

namespace naive {
namespace wm {

namespace {

std::string GetTime() {
  char buffer[80];
  auto t = std::time(nullptr);
  auto* tm = std::localtime(&t);
  std::strftime(buffer, 80, "%Y-%m-%d %H:%M:%S%p %A", tm);
  return std::string(buffer);
}

}  // namespace

TimeView::TimeView(int32_t x, int32_t y, int32_t width, int32_t height)
    : TextView(x, y, width, height) {
  TRACE("x: %d, y: %d, width: %d, height: %d", x, y, width, height);
  SetTextColor(0xFFFFFF00);
  SetTextSize(20);
}

void TimeView::OnDrawFrame() {
  // TODO: This requires the time view to be drawn every frame, which is not
  // needed... We only need the timer signal.
  TextView::OnDrawFrame();
  if (frame_ == 0)
    SetText(GetTime());
  frame_ = (frame_ + 1) % 60;
}

Panel::Panel(int32_t x, int32_t y, int32_t width, int32_t height)
    : TextView(x, y, width, height),
      time_view_(width / 2 - 500, 0, 500, height) {
  // TODO: scaling is not correct
  AddChild(&time_view_);

  SetTextColor(0xFF00FF00);
  SetTextSize(20);
  SetTextAlignment(ui::TextAlignment::CENTER_VERTICAL |
                   ui::TextAlignment::LEFT);

  SetText("<1>  2   3   4   5   6   7   8   9");
}

void Panel::OnWorkspaceChanged(int32_t workspace) {
  std::stringstream ss;

  for (int i = 0; i < 9; i++) {
    if (workspace == i)
      ss << "<" << i + 1 << "> ";
    else
      ss << " " << i + 1 << "  ";
  }
  SetText(ss.str());
}

}  // namespace wm
}  // namespace naive
