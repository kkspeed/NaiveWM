#include "wayland/display.h"

#include "wayland/pointer.h"

namespace naive {
namespace wayland {

Display::Display() {
  // TODO: initialize display.
}

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

void Display::NotifySurfaceCreated(Surface* surface, int32_t id) {
  for (auto* observer : surface_created_observers_)
    observer->OnSurfaceCreated(surface, id);
}

void Display::AddSurfaceCreatedObserver(SurfaceCreatedObserver* observer) {
  if (std::find(surface_created_observers_.begin(),
                surface_created_observers_.end(),
                observer) == surface_created_observers_.end()) {
    surface_created_observers_.push_back(observer);
  }
}

void Display::RemoveSurfaceCreatedObserver(SurfaceCreatedObserver* observer) {
  auto pos = std::find(surface_created_observers_.begin(),
                       surface_created_observers_.end(), observer);
  if (pos != surface_created_observers_.end())
    surface_created_observers_.erase(pos);
}

}  // namespace wayland
}  // namespace naive
