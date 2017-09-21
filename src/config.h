#ifndef CONFIG_H_
#define CONFIG_H_

#include <string>

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
// Layout string defines how the layout would be split
// synatx:
// |<0-100> refers to vertical split, with optional (0 - 100) / 100 partition of
//          the space
// -<0-100> refers to horizontal split, with optional (0 - 100) / 100 partition
//          of the space
// +        refers to recurse. It will repeat the action of last char or string
//          in the last paren.
// e.g.
// "(|50-50)+": Do a vertical half-half split. Then do a horizontal half-half
//              split. Repeat the action:
//           |---------------------------|
//           |            |              |
//           |            |              |            |
//           |            |--------------|
//           |            |      | ...   |
//           |------------|------|-------|
//           The above string can be simplified as "(|-)+".
// "|33(|-)+": Do a vertial 1 : 2 split. Then do the split on the rest of the
//             space as the above example:
//           |------|------|-------|
//           |      |      |       |
//           |      |      |       |
//           |      |      |       |
//           |      |      |---|---|
//           |      |      |   |   |
//           |      |      |   |---|
//           |      |      |   |...|
//           |------|------|---|---|
// If the string generates finite number of rectangles and they are fewer than
// the number of windows, the rest of the windows will be stacked on the last
// partition.
constexpr char kLayoutEqSplit[] = "(|-)+";
constexpr char kLayout12Split[] = "|33(|-)+";
constexpr char kLayout12SplitThenStack[] = "|33";
constexpr char kLayoutFullscreen[] = "";

// Layout for each tag.
const std::string kLayouts[] = {
    kLayoutEqSplit, kLayoutEqSplit, kLayoutEqSplit,
    kLayoutEqSplit, kLayoutEqSplit, kLayoutEqSplit,
    kLayoutEqSplit, kLayoutEqSplit, kLayoutFullscreen};

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
