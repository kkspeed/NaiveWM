#include "wm/manage/workspace.h"

namespace naive {
namespace wm {

ManageWindow::ManageWindow(Window* window, WMPrimitives* primitives)
  : window_(window), primitives_(primitives),
    is_floating_(window->is_transient() || window->is_popup()) {}

}  // namespace wm
}  // namespace naive
