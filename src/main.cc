#include "wayland/display.h"
#include "wayland/server.h"

#include <memory>

int main() {
  auto display = std::make_unique<naive::wayland::Display>();
  auto server = std::make_unique<naive::wayland::Server>(display.get());
  server->AddSocket();
  server->Run();
  return 0;
}
