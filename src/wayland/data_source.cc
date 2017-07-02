#include "wayland/data_source.h"

namespace naive {
namespace wayland {

DataSource::DataSource(wl_resource* resource) : resource_(resource) {}

DataSource::~DataSource() {
  for (auto* listener : listeners_)
    listener->OnDataSourceDestroyed(this);
}

void DataSource::Cancel() {
  wl_data_source_send_cancelled(resource_);
}

void DataSource::AddDataSourceListener(DataSourceListener* listener) {
  listeners_.insert(listener);
}

void DataSource::RemoveDataSourceListener(DataSourceListener* listener) {
  listeners_.erase(listener);
}

}  // namespace wayland
}  // namespace naive
