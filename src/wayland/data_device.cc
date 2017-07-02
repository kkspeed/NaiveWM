#include "wayland/data_device.h"

namespace naive {
namespace wayland {

DataDevice::~DataDevice() {
  // TODO: we may need to remove more listeners.
  if (selection_)
    selection_->RemoveDataSourceListener(this);
}

void DataDevice::OnDataSourceDestroyed(DataSource* source) {
  if (selection_ == source)
    selection_ = nullptr;
}

}  // namespace wayland
}  // namespace naive
