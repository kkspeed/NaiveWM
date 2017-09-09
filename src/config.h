#ifndef CONFIG_H_
#define CONFIG_H_

namespace naive {
namespace config {

const int32_t kX11WindowWidthPixels = 2200;
const int32_t kX11WindowHeightPixels = 1200;
const int32_t kX11WindowScaleFactor = 2;

// Change this to your wallpaper's path. Leave it empty to show a default
// background. Currently only PNG images are allowed.
constexpr char kWallpaperPath[] = "";

// Workspace insets.. used to place panels.
constexpr int32_t kWorkspaceInsetX = 0;
constexpr int32_t kWorkspaceInsetY = 10;

////////////////////////////////////////////////////////////////////////////////
// Panel configurations.
const int32_t kPanelTextSize = 16;
// Color(ARGB) of time color.
const int32_t kPanelTimeColor = 0xFFFFFF00;
// Workspace indicator color.
const int32_t kPanelWorkspaceIndicatorColor = 0xFF00FF00;

// Power indicator text color.
const int32_t kPowerIndicatorTextColor = 0xFF00FFFF;
// Power indicator background color normal
const int32_t kPowerIndicatorBackgroundColorNormal = 0xFF000000;
// Power indicator background color urgent
const int32_t kPowerIndicatorBackgroundColorUrgent = 0xFFDD0000;
// DBus UPowerinterface for battery.
const char kDBusBatteryInterface[] =
    "/org/freedesktop/UPower/devices/battery_BAT0";

}  // namespace config
}  // namespace naive

#endif
