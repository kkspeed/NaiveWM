#include "wayland/display.h"
#include "wayland/server.h"

#include <memory>

#include "compositor/compositor.h"
#include "wm/window_manager.h"

int main() {
  naive::wm::WindowManager::InitializeWindowManager();
  naive::compositor::Compositor::InitializeCompoistor();
  auto display = std::make_unique<naive::wayland::Display>();
  auto server = std::make_unique<naive::wayland::Server>(display.get());
  server->Run();
  return 0;
}
