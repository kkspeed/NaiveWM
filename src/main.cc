#include <poll.h>

#include "wayland/display.h"
#include "wayland/server.h"

#include "compositor/compositor.h"
#include "wm/window_manager.h"
#include "event/event_hub.h"

int main() {
  naive::event::EventHub::InitializeEventHub();
  naive::wm::WindowManager::InitializeWindowManager();
  naive::compositor::Compositor::InitializeCompoistor();

  auto display = std::make_unique<naive::wayland::Display>();
  auto server = std::make_unique<naive::wayland::Server>(display.get());
  int wayland_fd = server->GetFileDescriptor();

  auto* libinput = naive::event::EventHub::Get();
  int libinput_fd = libinput->GetFileDescriptor();
  pollfd fds[] = {{.fd = wayland_fd, .events = POLLIN},
                  {.fd = libinput_fd, .events = POLLIN}};

  for (;;) {
    server->DispatchEvents();
    libinput->HandleEvents();
    if (naive::compositor::Compositor::Get()->NeedToDraw())
      naive::compositor::Compositor::Get()->Draw();
    else
      poll(fds, 2, 3);
  }
  return 0;
}
