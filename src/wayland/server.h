#ifndef WAYLAND_SERVER_H_
#define WAYLAND_SERVER_H_

#include <wayland-server-protocol.h>
#include <memory>

#include "base/macros.h"
#include "wayland/display.h"
#include "wayland/seat.h"

extern "C" {
struct wl_display;
};

namespace naive {

namespace wayland {

template <class T>
T* GetUserDataAs(wl_resource* resource) {
  return static_cast<T*>(wl_resource_get_user_data(resource));
}

class Server {
 public:
  explicit Server(Display* display);
  void AddSocket();
  int GetFileDescriptor();
  void DispatchEvents();

  wl_display* wayland_display() { return wl_display_; }
  Display* display() { return display_; }

 private:
  Display* display_;
  wl_display* wl_display_;
  std::unique_ptr<Seat> seat_;

  DISALLOW_COPY_AND_ASSIGN(Server);
};

}  // namespace wayland
}  // namespace naive

#endif  // WAYLAND_SERVER_H_
