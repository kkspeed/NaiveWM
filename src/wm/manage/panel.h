#ifndef WM_MANAGE_PANEL_H_
#define WM_MANAGE_PANEL_H_

#include <vector>

#include "ui/text_view.h"
#include "wm/manage/power_indicator.h"

namespace naive {
namespace wm {

class TimeView : public ui::TextView {
 public:
  explicit TimeView(int32_t x, int32_t y, int32_t width, int32_t height);

  // ui::TextView overrides.
  void OnDrawFrame() override;

 private:
  int32_t frame_ = 0;
};

class Panel : public ui::TextView {
 public:
  explicit Panel(int32_t x, int32_t y, int32_t width, int32_t height);
  void OnWorkspaceChanged(int32_t workspace,
                          const std::vector<int32_t>& window_count);
  TimeView time_view_;
  extra::PowerIndicator power_indicator_;
};

}  // namespace wm
}  // namespace naive

#endif  // WM_MANAGE_PANEL_H_
