#include "compositor/compositor.h"

#include <cassert>

namespace naive {
namespace compositor {

Compositor* Compositor::g_compositor = nullptr;

// static
void Compositor::InitializeCompoistor() {
  g_compositor = new Compositor();
}

// static
Compositor* Compositor::Get() {
  assert(g_compositor);
  return g_compositor;
}

bool Compositor::Draw() {
  return false;
}

}  // namespace compositor
}  // namespace naive
