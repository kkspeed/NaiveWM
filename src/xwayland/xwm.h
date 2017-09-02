#ifndef XWAYLAND_XWM_H_
#define XWAYLAND_XWM_H_

#include <X11/Xlib.h>
#include <wayland-server-protocol.h>
#include <map>
#include <memory>

#include "base/geometry.h"
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
        wm_state(XInternAtom(display, "WM_STATE", 0)),
        wl_surface_id(XInternAtom(display, "WL_SURFACE_ID", 0)),
        net_active_window(XInternAtom(display, "_NET_ACTIVE_WINDOW", 0)),
        net_wm_window_type_dialog(
            XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", 0)),
        net_wm_window_type_desktop(
            XInternAtom(display, "_NET_WM_WINDOW_TYPE_DESKTOP", 0)),
        net_wm_window_type_dock(
            XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", 0)),
        net_wm_window_type_toolbar(
            XInternAtom(display, "_NET_WM_WINDOW_TYPE_TOOLBAR", 0)),
        net_wm_window_type_menu(
            XInternAtom(display, "_NET_WM_WINDOW_TYPE_MENU", 0)),
        net_wm_window_type_utility(
            XInternAtom(display, "_NET_WM_WINDOW_TYPE_UTILITY", 0)),
        net_wm_window_type_splash(
            XInternAtom(display, "_NET_WM_WINDOW_TYPE_SPLASH", 0)),
        net_wm_window_type_dropdown(
            XInternAtom(display, "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", 0)),
        net_wm_window_type_popup(
            XInternAtom(display, "_NET_WM_WINDOW_TYPE_POPUP_MENU", 0)),
        net_wm_window_type_tooltip(
            XInternAtom(display, "_NET_WM_WINDOW_TYPE_TOOLTIP", 0)),
        net_wm_window_type_notification(
            XInternAtom(display, "_NET_WM_WINDOW_TYPE_NOTIFICATION", 0)),
        net_wm_window_type_combo(
            XInternAtom(display, "_NET_WM_WINDOW_TYPE_COMBO", 0)),
        net_wm_window_type_dnd(
            XInternAtom(display, "_NET_WM_WINDOW_TYPE_DND", 0)),
        net_wm_window_type_normal(
            XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", 0)),
        net_wm_window_type(XInternAtom(display, "_NET_WM_WINDOW_TYPE", 0)),
        net_supported(XInternAtom(display, "_NET_SUPPORTED", 0)),
        allow_commits(XInternAtom(display, "_XWAYLAND_ALLOW_COMMITS", 0)) {}

  Atom wm_protocols;
  Atom wm_take_focus;
  Atom wm_delete;
  Atom wm_transient_for;
  Atom wm_state;
  Atom wl_surface_id;
  Atom net_active_window;

  Atom net_wm_window_type_dialog;
  Atom net_wm_window_type_desktop;
  Atom net_wm_window_type_dock;
  Atom net_wm_window_type_toolbar;
  Atom net_wm_window_type_menu;
  Atom net_wm_window_type_utility;
  Atom net_wm_window_type_splash;
  Atom net_wm_window_type_dropdown;
  Atom net_wm_window_type_popup;
  Atom net_wm_window_type_tooltip;
  Atom net_wm_window_type_notification;
  Atom net_wm_window_type_combo;
  Atom net_wm_window_type_dnd;
  Atom net_wm_window_type_normal;

  Atom net_wm_window_type;
  Atom net_supported;

  Atom allow_commits;
};

class XWindow {
 public:
  XWindow(Window window, std::unique_ptr<ShellSurface> shell_surface);
  ~XWindow() { TRACE("win: 0x%lx, %p", window_, this); }

  ShellSurface* shell_surface() { return shell_surface_.get(); }
  Window window() { return window_; }
  bool configured() { return configured_; }
  void set_configured(bool c) { configured_ = c; }
  void ReplaceShellSurface(std::unique_ptr<ShellSurface> shell_surface) {
    TRACE("replace shell surface for xwin: 0x%lx, %p -> %p", window_,
          shell_surface_.get(), shell_surface.get());
    shell_surface_->RecoverWindowState(shell_surface.get());
    shell_surface_ = std::move(shell_surface);
  }

 private:
  std::unique_ptr<ShellSurface> shell_surface_;
  bool configured_{false};
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
  void HandleConfigureNotify(XConfigureEvent* event);
  void HandleCreateNotify(XCreateWindowEvent* event);
  void HandleMapRequest(XMapRequestEvent* event);
  void HandleClientMessage(XClientMessageEvent* event);
  void HandlePropertyNotify(XPropertyEvent* event);
  void HandleDestroyNotify(XDestroyWindowEvent* event);
  void HandleUnmapNotify(XUnmapEvent* event);
  void HandleReparentNotify(XReparentEvent* event);

  void ConfigureEvent(Window window, base::geometry::Rect& rect);
  void ConfigureWindow(Window window, int32_t width, int32_t height);
  void FocusWindow(Window window);
  void SetClientState(Window window, long state);
  void KillWindow(Window window);
  void UpdateSizeHints(Window window, ShellSurface* shell_surface);

  ShellSurface* GetShellSurfaceByWindow(Window window);
  XWindow* GetXWindowByWindow(Window window);

  bool SendEvent(Window window, Atom proto);
  Atom GetAtomProp(Window window, Atom prop);

  Server* wayland_server_;
  wl_client* client_;

  XDisplay* x_display_;
  int screen_;
  Window root_;
  std::unique_ptr<Atoms> atoms_;
  std::vector<std::pair<Window, int32_t>> pending_surface_ids_;
  std::map<Window, base::geometry::Rect> pending_configureations_;
  std::vector<std::unique_ptr<XWindow>> x_windows_;
};

}  // namespace wayland
}  // namespace naive

#endif  // XWAYLAND_XWM_H_
