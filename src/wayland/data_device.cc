#include "wayland/data_device.h"
#include "wayland/seat.h"

namespace naive {
namespace wayland {

DataDevice::DataDevice(Seat* seat) : seat_(seat) {}

DataDevice::~DataDevice() {}

void DataDevice::OnDataSourceDestroyed(DataSource* source) {
  if (selection_ == source)
    selection_ = nullptr;
}

void DataDevice::set_selection(DataSource* selection) {
  selection_ = selection;
  seat_->OfferSelection();
}

}  // namespace wayland
}  // namespace naive
