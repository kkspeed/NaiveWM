#ifndef WM_MANAGE_POWER_INDICATOR_H_
#define WM_MANAGE_POWER_INDICATOR_H_

#include <cstdint>
#include <dbus/dbus.h>

#include "ui/text_view.h"

namespace naive {
namespace extra {

class DBusCon {
 public:
  explicit DBusCon();

  template <typename T>
  T GetProperty(const char* bus_name,
                const char* path,
                const char* interface,
                const char* prop);

  int64_t GetRemainingTime();
  double GetPercentage();

 private:
  void HandleError();

  DBusConnection* connection_{nullptr};
  DBusError error_;
};

class PowerIndicator: public ui::TextView {
public:
  explicit PowerIndicator(int32_t x, int32_t y, int32_t width, int32_t height);

  // ui::TextView overrides.
  void OnDrawFrame() override;
private:
  void UpdatePowerInfo();
  int32_t frame_{0};
  DBusCon dbus_;
};

}  // namespace extra
}  // namespace naive

#endif  // WM_MANAGE_POWER_INDICATOR_H_
