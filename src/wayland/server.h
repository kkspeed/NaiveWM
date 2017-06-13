#ifndef WAYLAND_SERVER_H_
#define WAYLAND_SERVER_H_

#include "base/macros.h"
#include "wayland/display.h"

extern "C" {
struct wl_display;
};

namespace naive {
namespace wayland {

class Server {
 public:
  explicit Server(Display* display);
  void AddSocket();
  int GetFileDescriptor();
  void DispatchEvents();

 private:
  Display* display_;
  wl_display* wl_display_;

  DISALLOW_COPY_AND_ASSIGN(Server);
};

}  // namespace wayland
}  // namespace naive

#endif  // WAYLAND_SERVER_H_
