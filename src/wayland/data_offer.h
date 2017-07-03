#ifndef WAYLAND_DATA_OFFER_H_
#define WAYLAND_DATA_OFFER_H_

#include <wayland-server-protocol.h>

#include "base/logging.h"
#include "wayland/data_source.h"

namespace naive {
namespace wayland {

class DataSource;

class DataOffer : DataSourceListener {
 public:
  explicit DataOffer(wl_resource* resource, DataSource* source)
      : resource_(resource), source_(source) {
    source->AddDataSourceListener(this);
  }
  ~DataOffer() {
    if (source_)
      source_->RemoveDataSourceListener(this);
  }

  DataSource* source() { return source_; }
  wl_resource* resource() { return resource_; }

  // DataSourceListener overrides
  void OnDataSourceDestroyed(DataSource* source) {
    if (source == source_) {
      TRACE();
      source_ = nullptr;
    }
  }

 private:
  wl_resource* resource_;
  DataSource* source_;
};

}  // namespace wayland
}  // namespace naive

#endif  // WAYLAND_DATA_OFFER_H_
