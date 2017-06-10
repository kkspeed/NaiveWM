#ifndef WM_WINDOW_MANAGER_H_
#define WM_WINDOW_MANAGER_H_

#include "wm/window.h"

namespace naive {
namespace wm {

class WindowManager {
 public:
  WindowManager();
  static void InitializeWindowManager();
  static WindowManager* Get();
  void Manage(Window* window);

 private:
  static WindowManager* g_window_manager;
};

}  // namespace wm
}  // namespace naive

#endif  // WM_WINDOW_MANAGER_H_
