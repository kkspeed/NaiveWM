#include "window_manager.h"

#include <cassert>
#include "base/logging.h"

namespace naive {
namespace wm {

// static
WindowManager* WindowManager::g_window_manager = nullptr;

// static
void WindowManager::InitializeWindowManager() {
  g_window_manager = new WindowManager();
}

WindowManager* WindowManager::Get() {
  assert(g_window_manager != nullptr);
  return g_window_manager;
}

WindowManager::WindowManager() {

}

void WindowManager::Manage(Window* window) {
  // TODO: window management policy
}

}  // namespace wm
}  // namespace naive
