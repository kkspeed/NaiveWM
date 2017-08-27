#include "xwayland/xwm.h"

#include <functional>
#include <signal.h>
#include <sys/socket.h>
#include <X11/Xproto.h>
#include <X11/extensions/Xcomposite.h>

#include "main_looper.h"
#include "wayland/display.h"
#include "wayland/server.h"

namespace naive {
namespace wayland {
namespace {

std::function<void()> sigusr_callback;

void SigUsr1(int num) {
  sigusr_callback();
}

int (*DefaultErrorHandler)(XDisplay* display, XErrorEvent* e);

int XErrorDummy(XDisplay* display, XErrorEvent* e) {
  return 0;
}

int XError(XDisplay* display, XErrorEvent* ee) {
  if (ee->error_code == BadWindow ||
      (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch) ||
      (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable) ||
      (ee->request_code == X_PolyFillRectangle &&
       ee->error_code == BadDrawable) ||
      (ee->request_code == X_PolySegment && ee->error_code == BadDrawable) ||
      (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch) ||
      (ee->request_code == X_GrabButton && ee->error_code == BadAccess) ||
      (ee->request_code == X_GrabKey && ee->error_code == BadAccess) ||
      (ee->request_code == X_CopyArea && ee->error_code == BadDrawable)) {
    return 0;
  }
  TRACE("NaiveWM: fatal error: request code=%d, error code=%d\n",
        ee->request_code, ee->error_code);
  return DefaultErrorHandler(display, ee); /* may call exit */
}

}  // namespace

XWindow::XWindow(Window window, std::unique_ptr<ShellSurface> shell_surface)
    : window_(window), shell_surface_(std::move(shell_surface)) {}

XWindowManager::XWindowManager(Server* server) : wayland_server_{server} {
  wayland_server_->display()->AddSurfaceCreatedObserver(this);
}

XWindowManager::~XWindowManager() {
  wayland_server_->display()->RemoveSurfaceCreatedObserver(this);
}

void XWindowManager::CreateManagedWindow(
    Window window,
    std::unique_ptr<ShellSurface> shell_surface) {
  int32_t buffer_scale = shell_surface->window()->surface()->buffer_scale();
  shell_surface->set_close_callback(
      [this, window]() { this->KillWindow(window); });
  shell_surface->set_configure_callback([this, window, buffer_scale](
      int32_t width, int32_t height) {
    this->ConfigureWindow(window, width * buffer_scale, height * buffer_scale);
    return 0;
  });
  // TODO: distinguish x window types
  shell_surface->window()->set_to_be_managed(true);
  shell_surface->window()->set_mouse_event_scale_override(1);
  x_windows_.push_back(
      std::make_unique<XWindow>(window, std::move(shell_surface)));
}

void XWindowManager::SpawnXServer() {
  int sv[2];
  socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv);
  pid_t pid = fork();
  if (pid == 0) {
    signal(SIGUSR1, SIG_IGN);
    int fd = dup(sv[1]);
    char s[8];
    snprintf(s, sizeof(s), "%d", fd);
    setenv("WAYLAND_SOCKET", s, 1);
    LOG(ERROR) << "Spawining XServer";
    if (execlp("Xwayland", "Xwayland", "-rootless", "+iglx", ":1", nullptr) <
        0) {
      LOG(ERROR) << "unable to spawn xserver";
      exit(1);
    }
    return;
  }
  close(sv[1]);
  client_ = wl_client_create(wayland_server_->wayland_display(), sv[0]);
  sigusr_callback = [this]() { this->OnXServerInitialized(); };
  signal(SIGUSR1, &SigUsr1);
}

int XWindowManager::GetFileDescriptor() {
  return ConnectionNumber(x_display_);
}

void XWindowManager::OnXServerInitialized() {
  x_display_ = XOpenDisplay(nullptr);
  assert(x_display_);
  TRACE("XDisplay: %p", x_display_);
  atoms_ = std::make_unique<Atoms>(x_display_);
  screen_ = DefaultScreen(x_display_);
  root_ = DefaultRootWindow(x_display_);

  XSetWindowAttributes wa;
  wa.event_mask =
      SubstructureNotifyMask | SubstructureRedirectMask | PropertyChangeMask;
  XChangeWindowAttributes(x_display_, root_, CWEventMask, &wa);
  XCompositeRedirectSubwindows(x_display_, root_, CompositeRedirectManual);

  DefaultErrorHandler = XSetErrorHandler(XError);
  MainLooper::Get()->AddFd(GetFileDescriptor(),
                           [this]() { this->HandleXEvents(); });
}

void XWindowManager::OnSurfaceCreated(Surface* surface, int32_t id) {
  TRACE("surface: %p, id: %d", surface, id);
  auto pos =
      std::find_if(pending_surface_ids_.begin(), pending_surface_ids_.end(),
                   [id](auto& p) { return p.second == id; });
  if (pos != pending_surface_ids_.end()) {
    Window window = (*pos).first;
    int32_t id = (*pos).second;
    pending_surface_ids_.erase(pos);
    CreateManagedWindow(
        window, wayland_server_->display()->CreateShellSurface(surface));
  }
}

void XWindowManager::HandleConfigureRequest(XEvent* event) {
  TRACE();
  XConfigureRequestEvent* ev = &event->xconfigurerequest;
  XWindowChanges wc;
  wc.x = ev->x;
  wc.y = ev->y;
  wc.width = ev->width;
  wc.height = ev->height;
  wc.border_width = ev->border_width;
  wc.sibling = ev->above;
  wc.stack_mode = ev->detail;
  XConfigureWindow(x_display_, ev->window, ev->value_mask, &wc);
  XSync(x_display_, 0);
}

void XWindowManager::HandleMapRequest(XMapRequestEvent* event) {
  TRACE();
  XMapWindow(x_display_, event->window);
  XSync(x_display_, 0);
}

void XWindowManager::HandleClientMessage(XClientMessageEvent* event) {
  if (event->message_type == atoms_->wl_surface_id) {
    int32_t surface_id = static_cast<int32_t>(event->data.l[0]);
    wl_resource* surface_resource = wl_client_get_object(client_, surface_id);
    if (surface_resource) {
      TRACE("Got surface id: %d, resource: %p", surface_id, surface_resource);
      auto* surface = GetUserDataAs<Surface>(surface_resource);
      CreateManagedWindow(
          event->window,
          wayland_server_->display()->CreateShellSurface(surface));
    } else {
      TRACE("Resource for surface id: %d has not been created yet", surface_id);
      pending_surface_ids_.push_back(
          std::pair<Window, int>(event->window, surface_id));
    }
  }
}

void XWindowManager::HandleDestroyNotify(XDestroyWindowEvent* event) {
  Window target = event->window;
  auto pos = std::find_if(x_windows_.begin(), x_windows_.end(),
                          [target](auto& w) { return w->window() == target; });
  if (pos != x_windows_.end()) {
    TRACE("Removing x_window: 0x%lx, shell_surface: %p", (*pos)->window(),
          (*pos)->shell_surface());
    x_windows_.erase(pos);
  }
}

void XWindowManager::HandleXEvents() {
  XEvent event;
  while (XPending(x_display_)) {
    XNextEvent(x_display_, &event);
    switch (event.type) {
      case ConfigureRequest:
        HandleConfigureRequest(&event);
      case MapRequest:
        HandleMapRequest(&event.xmaprequest);
      case ClientMessage:
        HandleClientMessage(&event.xclient);
      case DestroyNotify:
        HandleDestroyNotify(&event.xdestroywindow);
    }
  }
}

void XWindowManager::ConfigureWindow(Window window,
                                     int32_t width,
                                     int32_t height) {
  TRACE("width: %d, height: %d", width, height);
  if (width != 0 && height != 0) {
    XResizeWindow(x_display_, window, width, height);
    XSync(x_display_, 0);
  }
}

void XWindowManager::KillWindow(Window window) {
  if (!SendEvent(window, atoms_->wm_delete)) {
    XGrabServer(x_display_);
    auto* old_err_handler = XSetErrorHandler(XErrorDummy);
    XSetCloseDownMode(x_display_, DestroyAll);
    XKillClient(x_display_, window);
    XSync(x_display_, False);
    XSetErrorHandler(old_err_handler);
    XUngrabServer(x_display_);
  }
}

bool XWindowManager::SendEvent(Window window, Atom proto) {
  int n;
  Atom* protocols;
  int exists = 0;
  XEvent ev;

  if (XGetWMProtocols(x_display_, window, &protocols, &n)) {
    while (!exists && n--)
      exists = protocols[n] == proto;
    XFree(protocols);
  }
  if (exists) {
    ev.type = ClientMessage;
    ev.xclient.window = window;
    ev.xclient.message_type = atoms_->wm_protocols;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = proto;
    ev.xclient.data.l[1] = CurrentTime;
    XSendEvent(x_display_, window, False, NoEventMask, &ev);
  }
  return exists;
}

}  // namespace wayland
}  // namespace naive
