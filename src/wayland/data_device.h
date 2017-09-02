#ifndef WAYLAND_DATA_DEVICE_H_
#define WAYLAND_DATA_DEVICE_H_

#include <wayland-server-protocol.h>
#include <set>

#include "wayland/data_source.h"

namespace naive {
namespace wayland {

class Seat;

class DataDevice : public DataSourceListener {
 public:
  explicit DataDevice(Seat* seat);
  ~DataDevice();
  DataSource* selection() { return selection_; }
  void set_selection(DataSource* selection);

  void AddClient(wl_resource* resource) { bindings_.insert(resource); }
  void Unbind(wl_resource* resource) { bindings_.erase(resource); }
  wl_resource* GetBinding(wl_client* client) {
    for (auto iter = bindings_.begin(); iter != bindings_.end(); iter++) {
      if (wl_resource_get_client(*iter) == client)
        return *iter;
    }
    return nullptr;
  }

  // DataSourceListener overrides
  void OnDataSourceDestroyed(DataSource* source) override;

 private:
  DataSource* selection_;
  Seat* seat_;
  std::set<wl_resource*> bindings_;
};

}  // namespace wayland
}  // namespace naive

#endif  // WAYLAND_DATA_DEVICE_H_
