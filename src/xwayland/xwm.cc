#include "xwayland/xwm.h"

#include <functional>
#include <signal.h>
#include <sys/socket.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
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
  shell_surface->set_activation_callback(
      [this, window]() { this->FocusWindow(window); });
  shell_surface->window()->set_mouse_event_scale_override(1);

  if (!AdjustWindowFlags(window, shell_surface.get()))
    shell_surface->window()->set_to_be_managed(true);
  if (shell_surface->window()->is_popup() ||
      shell_surface->window()->is_transient()) {
    UpdateSizeHints(window, shell_surface.get());
  }

  auto pos =
      std::find_if(x_windows_.begin(), x_windows_.end(),
                   [window](auto& xwin) { return xwin->window() == window; });
  if (pos != x_windows_.end()) {
    (*pos)->ReplaceShellSurface(std::move(shell_surface));
  } else {
    x_windows_.push_back(
        std::make_unique<XWindow>(window, std::move(shell_surface)));
  }
}

bool XWindowManager::AdjustWindowFlags(Window window,
                                       ShellSurface* shell_surface) {
  base::geometry::Rect rect;
  if (pending_configureations_.find(window) != pending_configureations_.end()) {
    rect = pending_configureations_[window];
    pending_configureations_.erase(window);
  }
  if (!shell_surface->window())
    return true;

  XWindowAttributes xa;
  if (!XGetWindowAttributes(x_display_, window, &xa)) {
    TRACE("No attributes for xwin: 0x%lx, window: %p, treating as transient.",
          window, shell_surface->window())
    shell_surface->window()->set_transient(true);
    return false;
  }

  Atom window_type = GetAtomProp(window, atoms_->net_wm_window_type);
  Window transient_for_window;
  XGetTransientForHint(x_display_, window, &transient_for_window);
  ShellSurface* parent = GetShellSurfaceByWindow(transient_for_window);
  if (transient_for_window && parent) {
    if (!rect.Empty()) {
      TRACE("xwin: 0x%lx, window: %p, geometry: %s", window,
            shell_surface->window(), rect.ToString().c_str());
      int scale = shell_surface->window()->surface()->buffer_scale();
      shell_surface->SetGeometry(rect / scale);
    }
    if (window_type == atoms_->net_wm_window_type_tooltip ||
        window_type == atoms_->net_wm_window_type_dropdown ||
        window_type == atoms_->net_wm_window_type_dnd ||
        window_type == atoms_->net_wm_window_type_combo ||
        window_type == atoms_->net_wm_window_type_popup ||
        window_type == atoms_->net_wm_window_type_utility) {
      TRACE(
          "inactive transient for xwin: 0x%lx, window: %p, treating as "
          "transient for "
          "0x%lx, window: %p",
          window, shell_surface->window(), transient_for_window,
          parent->window());
      parent->window()->AddChild(shell_surface->window());
      return true;
    }
    parent->window()->AddChild(shell_surface->window());
    return true;
  }
  if ((xa.override_redirect || transient_for_window == root_) &&
      !rect.Empty()) {
    TRACE(
        "xwin: 0x%lx is transient for root or is override redirect, set "
        "geometry to: %s",
        window, rect.ToString().c_str());
    shell_surface->SetGeometry(
        rect / shell_surface->window()->surface()->buffer_scale());
    shell_surface->window()->set_popup(true);
    return false;
  }
  if (xa.override_redirect) {
    shell_surface->window()->set_popup(true);
    return false;
  }
  if (window_type == atoms_->net_wm_window_type_dialog ||
      window_type == atoms_->net_wm_window_type_dock) {
    TRACE("dialog xwin: 0x%lx, window: %p, treating as popup.", window,
          shell_surface->window())
    shell_surface->window()->set_popup(true);
    if (!rect.Empty()) {
      shell_surface->SetGeometry(
          rect / shell_surface->window()->surface()->buffer_scale());
    }
    if (window_type == atoms_->net_wm_window_type_dock)
      shell_surface->window()->override_border(true, false);
    return false;
  }
  return false;
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

  Atom net_atoms[] = {
      atoms_->net_active_window,
      atoms_->net_supported,
      atoms_->net_wm_window_type,
      atoms_->net_wm_window_type_dialog,
      atoms_->net_wm_window_type_desktop,
      atoms_->net_wm_window_type_dock,
      atoms_->net_wm_window_type_toolbar,
      atoms_->net_wm_window_type_menu,
      atoms_->net_wm_window_type_utility,
      atoms_->net_wm_window_type_splash,
      atoms_->net_wm_window_type_dropdown,
      atoms_->net_wm_window_type_popup,
      atoms_->net_wm_window_type_tooltip,
      atoms_->net_wm_window_type_notification,
      atoms_->net_wm_window_type_combo,
      atoms_->net_wm_window_type_dnd,
      atoms_->net_wm_window_type_normal,
  };
  XChangeProperty(x_display_, root_, atoms_->net_supported, XA_ATOM, 32,
                  PropModeReplace, (uint8_t*)net_atoms,
                  sizeof(net_atoms) / sizeof(Atom));

  XSetWindowAttributes wa;
  wa.cursor = XCreateFontCursor(x_display_, XC_left_ptr);
  wa.event_mask = StructureNotifyMask | SubstructureNotifyMask |
                  SubstructureRedirectMask | PropertyChangeMask;
  XChangeWindowAttributes(x_display_, root_, CWEventMask | CWCursor, &wa);
  XSelectInput(x_display_, root_, wa.event_mask);
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
  XConfigureRequestEvent* ev = &event->xconfigurerequest;
  TRACE("window: 0x%lx, (%d, %d), width: %d, height: %d", ev->window, ev->x,
        ev->y, ev->width, ev->height);
  XWindowChanges wc;
  wc.x = ev->x;
  wc.y = ev->y;
  wc.width = ev->width;
  wc.height = ev->height;
  wc.border_width = 0;  // ev->border_width;
  wc.sibling = ev->above;
  wc.stack_mode = ev->detail;
  ev->value_mask &= ~CWStackMode;
  ev->value_mask &= ~CWSibling;

  XConfigureWindow(x_display_, ev->window, ev->value_mask, &wc);
  XSync(x_display_, 0);
}

void XWindowManager::HandleCreateNotify(XCreateWindowEvent* event) {
  TRACE("xwin: 0x%lx, (%d, %d), w: %d, h: %d", event->window, event->x,
        event->y, event->width, event->height);
  auto* shell_surface = GetShellSurfaceByWindow(event->window);
  base::geometry::Rect r(event->x, event->y, event->width, event->height);
  if (!shell_surface) {
    pending_configureations_[event->window] = r;
    return;
  }
  if (!shell_surface->window())
    return;
  if (shell_surface->window()->is_popup() ||
      shell_surface->window()->is_transient()) {
    auto rect_dp = r / shell_surface->window()->surface()->buffer_scale();
    shell_surface->SetGeometry(rect_dp);
    shell_surface->window()->PushProperty(true, rect_dp.x(), rect_dp.y());
  }
}

void XWindowManager::HandleMapRequest(XMapRequestEvent* event) {
  TRACE("xwin: 0x%lx", event->window);
  uint32_t property[1] = {0};
  XChangeProperty(x_display_, event->window, atoms_->allow_commits, XA_CARDINAL,
                  32, PropModeReplace, (uint8_t*)property, 1);
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
      TRACE(
          "Resource for surface id: %d (window: 0x%lx) has not been created "
          "yet",
          surface_id, event->window);
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

void XWindowManager::HandleUnmapNotify(XUnmapEvent* event) {
  Window target = event->window;
  auto pos = std::find_if(x_windows_.begin(), x_windows_.end(),
                          [target](auto& w) { return w->window() == target; });
  if (pos != x_windows_.end()) {
    TRACE("Removing x_window: 0x%lx, shell_surface: %p", (*pos)->window(),
          (*pos)->shell_surface());
    if (event->send_event) {
      SetClientState(event->window, WithdrawnState);
    } else
      x_windows_.erase(pos);
  }
}

void XWindowManager::HandleConfigureNotify(XConfigureEvent* event) {
  Window window = event->window;

  ShellSurface* shell_surface = GetShellSurfaceByWindow(window);
  TRACE("window: 0x%lx, shell_surface: %p, (%d, %d), %d x %d", window,
        shell_surface, event->x, event->y, event->width, event->height);
  if (!shell_surface) {
    // ShellSurface has not been created.. we need to save this pending
    // configuration and apply it once shell surface is created.
    pending_configureations_[event->window] =
        base::geometry::Rect(event->x, event->y, event->width, event->height);
    return;
  }
  if (!shell_surface->window())
    return;
  if (shell_surface->window()->is_popup() ||
      shell_surface->window()->is_transient()) {
    auto rect_dp =
        base::geometry::Rect(event->x, event->y, event->width, event->height) /
        shell_surface->window()->surface()->buffer_scale();
    shell_surface->SetGeometry(rect_dp);
    shell_surface->window()->PushProperty(true, rect_dp.x(), rect_dp.y());
    return;
  }
}

void XWindowManager::HandlePropertyNotify(XPropertyEvent* event) {
  Window window = event->window;
  TRACE("xwin: 0x%lx", window);
  if (event->state == PropertyDelete)
    return;

  if (window == root_) {
    // TODO: Root probably handles WM_NAME.
    return;
  }
  if (event->atom == XA_WM_NORMAL_HINTS) {
    ShellSurface* shell_surface = GetShellSurfaceByWindow(window);
    UpdateSizeHints(window, shell_surface);
    return;
  }

  if (event->atom == XA_WM_TRANSIENT_FOR) {
    Window parent = 0;
    if (!XGetTransientForHint(x_display_, window, &parent))
      return;
    TRACE("window: 0x%lx hints as transient for 0x%lx", window, parent);
    return;
  }
}

void XWindowManager::HandleReparentNotify(XReparentEvent* event) {
  Window xwin = event->window;
  TRACE("xwin: 0x%lx", xwin);
  auto pos = std::find_if(x_windows_.begin(), x_windows_.end(),
                          [xwin](auto& w) { return w->window() == xwin; });
  if (pos != x_windows_.end())
    x_windows_.erase(pos);
}

void XWindowManager::HandleXEvents() {
  XEvent event;
  while (XPending(x_display_)) {
    XNextEvent(x_display_, &event);
    switch (event.type) {
      case ConfigureRequest:
        HandleConfigureRequest(&event);
        break;
      case CreateNotify:
        HandleCreateNotify(&event.xcreatewindow);
        break;
      case ConfigureNotify:
        HandleConfigureNotify(&event.xconfigure);
        break;
      case ReparentNotify:
        HandleReparentNotify(&event.xreparent);
        break;
      case MapRequest:
        HandleMapRequest(&event.xmaprequest);
        break;
      case ClientMessage:
        HandleClientMessage(&event.xclient);
        break;
      case PropertyNotify:
        HandlePropertyNotify(&event.xproperty);
        break;
      case UnmapNotify:
        HandleUnmapNotify(&event.xunmap);
        break;
      case DestroyNotify:
        HandleDestroyNotify(&event.xdestroywindow);
        break;
    }
  }
}

void XWindowManager::ConfigureWindow(Window window,
                                     int32_t width,
                                     int32_t height) {
  XWindow* xwin = GetXWindowByWindow(window);
  if (xwin) {
    TRACE("0x%lx, width: %d, height: %d, wl_window: %p, surface: %p", window,
          width, height, xwin->shell_surface()->window(),
          xwin->shell_surface());
  }

  if (width != 0 && height != 0 && xwin) {
    XResizeWindow(x_display_, window, width, height);
    if (!xwin->configured()) {
      uint32_t property[1] = {0};
      XChangeProperty(x_display_, window, atoms_->allow_commits, XA_CARDINAL,
                      32, PropModeReplace, (uint8_t*)property, 1);
      xwin->set_configured(true);
    }
    XSync(x_display_, 0);
  }
}

void XWindowManager::FocusWindow(Window window) {
  TRACE("Focusing: xwindow: 0x%lx", window);
  XSetInputFocus(x_display_, window, RevertToPointerRoot, CurrentTime);
  XChangeProperty(x_display_, root_, atoms_->net_active_window, XA_WINDOW, 32,
                  PropModeReplace, (unsigned char*)&(window), 1);
  XRaiseWindow(x_display_, window);
  SendEvent(window, atoms_->wm_take_focus);
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

Atom XWindowManager::GetAtomProp(Window window, Atom prop) {
  int di;
  unsigned long dl;
  unsigned char* p = NULL;
  Atom da, atom = None;

  if (XGetWindowProperty(x_display_, window, prop, 0L, sizeof atom, False,
                         XA_ATOM, &da, &di, &dl, &dl, &p) == Success &&
      p) {
    atom = *(Atom*)p;
    XFree(p);
  }
  return atom;
}

void XWindowManager::ConfigureEvent(Window window, base::geometry::Rect& rect) {
  TRACE("window: 0x%lx, geometry: %s", window, rect.ToString().c_str());
  XConfigureEvent ce;
  ce.type = ConfigureNotify;
  ce.display = x_display_;
  ce.event = window;
  ce.window = window;
  ce.x = rect.x();
  ce.y = rect.y();
  ce.width = rect.width();
  ce.height = rect.height();
  ce.border_width = 0;
  ce.above = None;
  ce.override_redirect = False;
  XSendEvent(x_display_, window, False, StructureNotifyMask, (XEvent*)&ce);
}

void XWindowManager::UpdateSizeHints(Window window,
                                     ShellSurface* shell_surface) {
  TRACE("xwin: 0x%lx, shell_surface: %p", window, shell_surface);
  long msize;
  XSizeHints size;
  if (!XGetWMNormalHints(x_display_, window, &size, &msize))
    return;
  base::geometry::Rect rect;
  if (size.flags & PPosition) {
    rect.x_ = size.x;
    rect.y_ = size.y;
  }
  if (size.flags & PSize) {
    rect.width_ = size.width;
    rect.height_ = size.height;
  }
  if (rect.Empty())
    return;
  TRACE("xwin: 0x%lx, size hint: %s", window, rect.ToString().c_str());
  if (!shell_surface) {
    TRACE("shell surface for window 0x%lx not created yet...", window);
    pending_configureations_.emplace(window, rect);
    return;
  }
  if (!shell_surface->window())
    return;
  auto rect_dp = rect / shell_surface->window()->surface()->buffer_scale();
  shell_surface->SetGeometry(rect_dp);
  shell_surface->window()->PushProperty(true, rect_dp.x(), rect_dp.y());
}

void XWindowManager::SetClientState(Window window, long state) {
  long data[] = {state, None};
  XChangeProperty(x_display_, window, atoms_->wm_state, atoms_->wm_state, 32,
                  PropModeReplace, (unsigned char*)data, 2);
}

ShellSurface* XWindowManager::GetShellSurfaceByWindow(Window window) {
  auto pos = std::find_if(x_windows_.begin(), x_windows_.end(),
                          [window](auto& w) { return w->window() == window; });
  return pos == x_windows_.end() ? nullptr : (*pos)->shell_surface();
}

XWindow* XWindowManager::GetXWindowByWindow(Window window) {
  auto pos = std::find_if(x_windows_.begin(), x_windows_.end(),
                          [window](auto& w) { return w->window() == window; });
  return pos == x_windows_.end() ? nullptr : (*pos).get();
}

}  // namespace wayland
}  // namespace naive
