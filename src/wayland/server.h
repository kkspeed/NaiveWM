#ifndef WAYLAND_SERVER_H_
#define WAYLAND_SERVER_H_

#include "base/macros.h"

namespace naive {
namespace wayland {

class Server {
 public:
  explicit Server();

 private:
  DISALLOW_COPY_AND_ASSIGN(Server);
};

}  // namespace wayland
}  // namespace naive

#endif  // WAYLAND_SERVER_H_
