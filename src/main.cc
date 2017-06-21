#include <poll.h>

#include "wayland/display.h"
#include "wayland/server.h"

#include "compositor/compositor.h"
#include "event/event_hub.h"
#include "wm/manage_hook.h"
#include "wm/window_manager.h"

int main() {
  naive::event::EventHub::InitializeEventHub();
  naive::wm::WindowManager::InitializeWindowManager(new naive::wm::ManageHook);
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
    naive::compositor::Compositor::Get()->Draw();
    poll(fds, 2, 3);
    // else
    // if (naive::compositor::Compositor::Get()->NeedToDraw())
  }
  return 0;
}
