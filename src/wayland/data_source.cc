#include "wayland/data_source.h"

#include "base/logging.h"

namespace naive {
namespace wayland {

DataSource::DataSource(wl_resource* resource) : resource_(resource) {}

DataSource::~DataSource() {
  for (auto* listener : listeners_)
    listener->OnDataSourceDestroyed(this);
}

void DataSource::Cancel() {
  TRACE("%p, resource: %p", this, resource_);
  wl_data_source_send_cancelled(resource_);
}

void DataSource::AddDataSourceListener(DataSourceListener* listener) {
  listeners_.insert(listener);
}

void DataSource::RemoveDataSourceListener(DataSourceListener* listener) {
  listeners_.erase(listener);
}

void DataSource::AddMimetype(std::string mimetype) {
  mimetypes_.insert(mimetype);
}

}  // namespace wayland
}  // namespace naive
