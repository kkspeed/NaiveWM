#include "backend/x11_backend/x11_backend.h"

#include <cstdint>
#include <linux/input.h>

#include "backend/egl_context.h"
#include "wayland/display_metrics.h"

namespace naive {
namespace backend {

namespace {
const int32_t kWidthPixels = 2560;
const int32_t kHeightPixels = 1440;
const int32_t kScaleFactor = 2;
const int32_t k1xPixelPerMm = 4;
}  // namespace

X11Backend::X11Backend(const char* display) {
  x_display_ = XOpenDisplay(display);
  XSetWindowAttributes swa;
  swa.event_mask = ExposureMask | PointerMotionMask | ButtonPressMask |
                   ButtonReleaseMask | KeyPressMask | KeyReleaseMask;

  x_window_ = XCreateWindow(x_display_, DefaultRootWindow(x_display_), 0, 0,
                            kWidthPixels, kHeightPixels, 0, CopyFromParent,
                            InputOutput, CopyFromParent, CWEventMask, &swa);

  XSetWindowAttributes xattr;
  Atom atom;
  xattr.override_redirect = False;

  XChangeWindowAttributes(x_display_, x_window_, CWOverrideRedirect, &xattr);
  XWMHints hints;
  hints.input = True;
  hints.flags = InputHint;
  XSetWMHints(x_display_, x_window_, &hints);
  XMapWindow(x_display_, x_window_);
  XStoreName(x_display_, x_window_, "NaiveWM");

  egl_ = std::make_unique<EglContext>(x_display_, (void*)x_window_,
                                      EGL_PLATFORM_X11_KHR);
  display_metrics_ = std::make_unique<wayland::DisplayMetrics>(
      kWidthPixels, kHeightPixels,
      kWidthPixels / (kScaleFactor * k1xPixelPerMm),
      kHeightPixels / (kScaleFactor * k1xPixelPerMm));
  egl_->CreateDrawBuffer(display_metrics_->width_pixels,
                         display_metrics_->height_pixels);
  egl_->SwapBuffers();

  last_mouse_x_ = display_metrics_->width_pixels / 2;
  last_mouse_y_ = display_metrics_->height_pixels / 2;
}

void X11Backend::FinalizeDraw(bool did_draw) {
  if (did_draw)
    egl_->SwapBuffers();
}

void X11Backend::AddHandler(base::Looper* handler) {
  handler->AddFd(ConnectionNumber(x_display_),
                 [this]() { this->DispatchEvents(); });
}

void X11Backend::HandleMotionEvent(XMotionEvent* event) {
  int32_t dx = event->x - last_mouse_x_;
  int32_t dy = event->y - last_mouse_y_;
  last_mouse_x_ = event->x;
  last_mouse_y_ = event->y;
  // TODO: we don't use LEDs yet..
  for (auto* observer : observers_)
    observer->OnMouseMotion(dx, dy, GetModifiers(event->state),
                            static_cast<event::Leds>(0));
}

void X11Backend::HandleButtonEvent(XButtonEvent* event) {
  uint32_t button = GetButton(event->button);
  for (auto* observer : observers_) {
    if (button == Button4 || button == Button5) {
      // TODO: This does not work with ThinkPad...
      observer->OnMouseScroll(button == Button4 ? -2 : 2, 0,
                              GetModifiers(event->state),
                              static_cast<event::Leds>(0));
    } else {
      observer->OnMouseButton(
          GetButton(event->button), event->type == ButtonPress,
          GetModifiers(event->state), static_cast<event::Leds>(0));
    }
  }
}

void X11Backend::HandleKeyEvent(XKeyEvent* event) {
  TRACE("keycode: %x", event->keycode - 8);
  for (auto* observer : observers_)
    observer->OnKey(event->keycode - 8, GetModifiers(event->state),
                    event->type == KeyPress, static_cast<event::Leds>(0));
}

uint32_t X11Backend::GetButton(uint32_t button) {
  switch (button) {
    case Button1:
      return BTN_LEFT;
    case Button2:
      return BTN_MIDDLE;
    case Button3:
      return BTN_RIGHT;
  }

  LOG_ERROR << "Unknown button: %d" << button;
  return 0;
}

uint32_t X11Backend::GetModifiers(uint32_t state) {
  uint32_t modifiers = 0;
  if (state & ControlMask)
    modifiers |= event::KeyModifiers::CONTROL;
  if (state & ShiftMask)
    modifiers |= event::KeyModifiers::SHIFT;
  // Swap Alt and Super so that they won't conflict with
  // my tiling window manager.
  if (state & Mod1Mask)
    modifiers |= event::KeyModifiers::SUPER;
  if (state & Mod4Mask)
    modifiers |= event::KeyModifiers::ALT;
  return modifiers;
}

void X11Backend::DispatchEvents() {
  XEvent event;
  while (XPending(x_display_)) {
    XNextEvent(x_display_, &event);
    switch (event.type) {
      case ButtonPress:
      case ButtonRelease:
        HandleButtonEvent(&event.xbutton);
        break;
      case MotionNotify:
        HandleMotionEvent(&event.xmotion);
        break;
      case KeyPress:
      case KeyRelease:
        HandleKeyEvent(&event.xkey);
        break;
    }
  }
}

}  // namespace event
}  // namespace naive
