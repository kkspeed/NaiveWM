#include "wm/manage/panel.h"

#include <sstream>
#include <string>

#include "base/logging.h"
#include "base/time.h"
#include "config.h"

namespace naive {
namespace wm {

TimeView::TimeView(int32_t x, int32_t y, int32_t width, int32_t height)
    : TextView(x, y, width, height) {
  TRACE("x: %d, y: %d, width: %d, height: %d", x, y, width, height);
  SetTextColor(config::kPanelTimeColor);
  SetTextSize(config::kPanelTextSize);
}

void TimeView::OnDrawFrame() {
  // TODO: This requires the time view to be drawn every frame, which is not
  // needed... We only need the timer signal.
  TextView::OnDrawFrame();
  if (frame_ == 0)
    SetText(base::Time::GetTime("%Y-%m-%d %H:%M:%S%p %A"));
  frame_ = (frame_ + 1) % 60;
}

Panel::Panel(int32_t x, int32_t y, int32_t width, int32_t height)
    : TextView(x, y, width, height),
      time_view_(width - 500, 0, 500, height),
      power_indicator_(width - 1000, 0, 400, height) {
  // TODO: scaling is not correct
  AddChild(&time_view_);
  AddChild(&power_indicator_);

  SetTextColor(config::kPanelWorkspaceIndicatorColor);
  SetTextSize(config::kPanelTextSize);
  SetTextAlignment(ui::TextAlignment::CENTER_VERTICAL |
                   ui::TextAlignment::LEFT);

  SetText("<1>");
}

void Panel::OnWorkspaceChanged(int32_t workspace,
                               const std::vector<int32_t>& window_count) {
  std::stringstream ss;

  for (int i = 0; i < 9; i++) {
    if (workspace == i) {
      ss << "<" << i + 1 << "> ";
      continue;
    }
    if (window_count[i] != 0)
      ss << " " << i + 1 << "  ";
  }
  SetText(ss.str());
}

}  // namespace wm
}  // namespace naive
