#ifndef WM_WINDOW_MANAGER_H_
#define WM_WINDOW_MANAGER_H_

#include <vector>

#include "wm/window.h"

namespace naive {
namespace wm {

class WindowManager {
 public:
  WindowManager();
  static void InitializeWindowManager();
  static WindowManager* Get();
  void Manage(Window* window);

  std::vector<Window*> windows() { return windows_; }

 private:
  static WindowManager* g_window_manager;
  std::vector<Window*> windows_;
};

}  // namespace wm
}  // namespace naive

#endif  // WM_WINDOW_MANAGER_H_
