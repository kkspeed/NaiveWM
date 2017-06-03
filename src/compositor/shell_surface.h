#ifndef COMPOSITOR_SHELL_SURFACE_H
#define COMPOSITOR_SHELL_SURFACE_H

#include "wm/window.h"

namespace naive {

class ShellSurface {
 public:
  void Move();
  wm::Window* GetWindow();
};

}  // namespace naive

#endif  // COMPOSITOR_SHELL_SURFACE_H
