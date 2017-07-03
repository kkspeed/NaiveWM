#ifndef WAYLAND_DATA_OFFER_H_
#define WAYLAND_DATA_OFFER_H_

#include <wayland-server-protocol.h>

namespace naive {
namespace wayland {

class DataSource;

class DataOffer {
 public:
  explicit DataOffer(wl_resource* resource, DataSource* source)
      : resource_(resource), source_(source) {}
  DataSource* source() { return source_; }
  wl_resource* resource() { return resource_; }

 private:
  wl_resource* resource_;
  DataSource* source_;
};

}  // namespace wayland
}  // namespace naive

#endif  // WAYLAND_DATA_OFFER_H_
