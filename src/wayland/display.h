#ifndef _WAYLAND_DISPLAY_H
#define _WAYLAND_DISPLAY_H

#include "compositor/surface.h"
#include "wayland/shared_memory.h"

namespace naive {
namespace wayland {

class Display {
 public:
  Display();

  std::unique_ptr<Surface> CreateSurface();
  std::unique_ptr<SharedMemory> CreateSharedMemory(int fd, int32_t size);
};

}  // namespace wayland
}  // namespace naive

#endif  // _WAYLAND_DISPLAY_H_
