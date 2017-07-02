#ifndef WAYLAND_DATA_DEVICE_H_
#define WAYLAND_DATA_DEVICE_H_

#include "wayland/data_source.h"

namespace naive {
namespace wayland {

class DataDevice : public DataSourceListener {
 public:
  ~DataDevice();
  DataSource* selection() { return selection_; };
  void set_selection(DataSource* selection) { selection_ = selection; }

  // DataSourceListener overrides
  void OnDataSourceDestroyed(DataSource* source) override;
 private:
  DataSource* selection_;
};

}  // namespace wayland
}  // namespace naive

#endif  // WAYLAND_DATA_DEVICE_H_
