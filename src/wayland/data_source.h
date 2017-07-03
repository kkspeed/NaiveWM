#ifndef WAYLAND_DATASOURCE_H_
#define WAYLAND_DATASOURCE_H_

#include <wayland-server-protocol.h>
#include <set>
#include <string>

namespace naive {
namespace wayland {

class DataSource;

class DataSourceListener {
 public:
  virtual void OnDataSourceDestroyed(DataSource* source) = 0;
};

class DataSource {
 public:
  explicit DataSource(wl_resource* resource);
  ~DataSource();

  void Cancel();

  void AddDataSourceListener(DataSourceListener* listener);
  void RemoveDataSourceListener(DataSourceListener* listener);

  void AddMimetype(std::string mimetype);
  std::set<std::string>& mimetypes() { return mimetypes_; }

  wl_resource* resource() { return resource_; }

 private:
  wl_resource* resource_;
  std::set<std::string> mimetypes_;
  std::set<DataSourceListener*> listeners_;
};

}  // namespace wayland
}  // namespace naive
#endif  // WAYLAND_DATASOURCE_H_
