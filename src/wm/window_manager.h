#ifndef WM_WINDOW_MANAGER_H_
#define WM_WINDOW_MANAGER_H_

#include "wm/window.h"

namespace naive {
namespace wm {

class WindowManager {
 public:
  static WindowManager* Get();
  void Manage(Window* window);
};

}  // namespace wm
}  // namespace naive

#endif  // WM_WINDOW_MANAGER_H_
