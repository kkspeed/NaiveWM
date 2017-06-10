#include "display.h"

namespace naive {
namespace wayland {

std::unique_ptr<ShellSurface> Display::CreateShellSurface(Surface* surface) {
  return std::make_unique<ShellSurface>(surface);
}

}  // namespace wayland
}  // namespace naive
