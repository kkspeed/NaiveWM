#include <poll.h>

#include "wayland/display.h"
#include "wayland/server.h"

#include "compositor/compositor.h"
#include "wm/window_manager.h"
#include "event/event_hub.h"

int main() {
  naive::event::EventHub::InitializeEventHub();
  pollfd fds;
  fds.fd = naive::event::EventHub::Get()->GetFileDescriptor();
  fds.events = POLLIN;
  fds.revents = 0;
  naive::wm::WindowManager::InitializeWindowManager();
  naive::compositor::Compositor::InitializeCompoistor();
  auto display = std::make_unique<naive::wayland::Display>();
  auto server = std::make_unique<naive::wayland::Server>(display.get());
  server->Run();
  return 0;
}
