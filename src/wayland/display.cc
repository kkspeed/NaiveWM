#include "display.h"

namespace naive {
namespace wayland {

std::unique_ptr<ShellSurface> Display::CreateShellSurface(Surface* surface) {
  return std::make_unique<ShellSurface>(surface);
}

std::unique_ptr<SharedMemory> Display::CreateSharedMemory(int fd,
                                                          int32_t size) {
  return std::make_unique<SharedMemory>(fd, size);
}

std::unique_ptr<SubSurface> Display::CreateSubSurface(Surface* surface,
                                                      Surface* parent) {
  return std::make_unique<SubSurface>(parent, surface);
}

std::unique_ptr<Surface> Display::CreateSurface() {
  return std::make_unique<Surface>();
}

}  // namespace wayland
}  // namespace naive
