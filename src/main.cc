#include <poll.h>
#include <memory>

#include "main_looper.h"
#include "wayland/display.h"
#include "wayland/server.h"

#include "backend/backend.h"
#include "backend/drm_backend/drm_backend.h"
#include "backend/x11_backend/x11_backend.h"
#include "compositor/compositor.h"
#include "wm/manage/manage_hook.h"
#include "wm/window_manager.h"
#include "xwayland/xwm.h"

int main(int argc, char* argv[]) {
  // Decide backend based on -x11 argument
  std::unique_ptr<naive::backend::Backend> backend;
  if (argc > 1 && strncmp(argv[1], "-x11", 5) == 0)
    backend = std::make_unique<naive::backend::X11Backend>();
  else
    backend = std::make_unique<naive::backend::DrmBackend>();

  naive::compositor::Compositor::InitializeCompoistor(backend.get());

  auto manage_hook = std::make_unique<naive::wm::ManageHook>();
  naive::wm::WindowManager::InitializeWindowManager(manage_hook.get(),
                                                    backend->GetEventHub());
  manage_hook->PostSetupPolicy();

  auto display = std::make_unique<naive::wayland::Display>();
  auto server = std::make_unique<naive::wayland::Server>(display.get());
  int wayland_fd = server->GetFileDescriptor();

#ifndef NO_XWAYLAND
  auto xwm = std::make_unique<naive::wayland::XWindowManager>(server.get());
  xwm->SpawnXServer();
#endif

  auto* looper = naive::MainLooper::Get();
  backend->AddHandler(looper);
  looper->AddFd(wayland_fd, [&server]() { server->DispatchEvents(); });
  looper->AddHandler([]() { naive::compositor::Compositor::Get()->Draw(); });
  looper->Run();

  return 0;
}
