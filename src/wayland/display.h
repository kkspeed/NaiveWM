#ifndef WAYLAND_DISPLAY_H_
#define WAYLAND_DISPLAY_H_

#include <wayland-server.h>

#include "compositor/shell_surface.h"
#include "compositor/subsurface.h"
#include "compositor/surface.h"
#include "wayland/shared_memory.h"

namespace naive {
namespace wayland {

class Pointer;

class Display {
 public:
  Display();

  std::unique_ptr<Surface> CreateSurface();
  std::unique_ptr<SubSurface> CreateSubSurface(Surface* surface,
                                               Surface* parent);
  std::unique_ptr<SharedMemory> CreateSharedMemory(int fd, int32_t size);
  std::unique_ptr<ShellSurface> CreateShellSurface(Surface* surface);
};

}  // namespace wayland
}  // namespace naive

#endif  // WAYLAND_DISPLAY_H_
