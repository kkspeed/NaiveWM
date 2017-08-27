#ifndef XWAYLAND_XWM_H_
#define XWAYLAND_XWM_H_

#include <X11/Xlib.h>
#include <wayland-server-protocol.h>
#include <memory>

#include "compositor/shell_surface.h"
#include "wayland/display.h"

namespace naive {
namespace wayland {

using XDisplay = ::Display;

class Server;

class Atoms {
 public:
  explicit Atoms(XDisplay* display)
      : wm_protocols(XInternAtom(display, "WM_PROTOCOLS", 0)),
        wm_take_focus(XInternAtom(display, "WM_TAKE_FOCUS", 0)),
        wm_delete(XInternAtom(display, "WM_DELETE_WINDOW", 0)),
        wm_transient_for(XInternAtom(display, "WM_TRANSIENT_FOR", 0)),
        wl_surface_id(XInternAtom(display, "WL_SURFACE_ID", 0)),
        net_active_window(XInternAtom(display, "_NET_ACTIVE_WINDOW", 0)),
        net_wm_window_type_dialog(
            XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", 0)),
        net_wm_window_type(XInternAtom(display, "_NET_WM_WINDOW_TYPE", 0)),
        net_supported(XInternAtom(display, "_NET_SUPPORTED", 0)),
        allow_commits(XInternAtom(display, "_XWAYLAND_ALLOW_COMMITS", 0)) {}

  Atom wm_protocols;
  Atom wm_take_focus;
  Atom wm_delete;
  Atom wm_transient_for;
  Atom wl_surface_id;
  Atom net_active_window;
  Atom net_wm_window_type_dialog;
  Atom net_wm_window_type;
  Atom net_supported;
  Atom allow_commits;
};

class XWindow {
 public:
  XWindow(Window window, std::unique_ptr<ShellSurface> shell_surface);

  ShellSurface* shell_surface() { return shell_surface_.get(); }
  Window window() { return window_; }

 private:
  std::unique_ptr<ShellSurface> shell_surface_;
  Window window_;
};

class XWindowManager : public SurfaceCreatedObserver {
 public:
  explicit XWindowManager(Server* server);
  ~XWindowManager();

  void SpawnXServer();
  int GetFileDescriptor();
  void OnXServerInitialized();

  void OnSurfaceCreated(Surface* surface, int32_t id) override;

 private:
  void CreateManagedWindow(Window window,
                           std::unique_ptr<ShellSurface> shell_surface);
  bool AdjustWindowFlags(Window, ShellSurface* shell_surface);

  void HandleXEvents();

  void HandleConfigureRequest(XEvent* event);
  void HandleMapRequest(XMapRequestEvent* event);
  void HandleClientMessage(XClientMessageEvent* event);
  void HandleDestroyNotify(XDestroyWindowEvent* event);

  void ConfigureWindow(Window window, int32_t width, int32_t height);
  void FocusWindow(Window window);
  void KillWindow(Window window);

  ShellSurface* GetShellSurfaceByWindow(Window window);

  bool SendEvent(Window window, Atom proto);
  Atom GetAtomProp(Window window, Atom prop);

  Server* wayland_server_;
  wl_client* client_;

  XDisplay* x_display_;
  int screen_;
  Window root_;
  std::unique_ptr<Atoms> atoms_;
  std::vector<std::pair<Window, int32_t>> pending_surface_ids_;
  std::vector<std::unique_ptr<XWindow>> x_windows_;
};

}  // namespace wayland
}  // namespace naive

#endif  // XWAYLAND_XWM_H_
