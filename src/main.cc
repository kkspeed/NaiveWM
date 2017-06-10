#include "wayland/display.h"
#include "wayland/server.h"

#include <memory>

#include "wm/window_manager.h"

int main() {
  naive::wm::WindowManager::InitializeWindowManager();
  auto display = std::make_unique<naive::wayland::Display>();
  auto server = std::make_unique<naive::wayland::Server>(display.get());
  server->AddSocket();
  server->Run();
  return 0;
}
