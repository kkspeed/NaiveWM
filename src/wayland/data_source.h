#ifndef WAYLAND_DATASOURCE_H_
#define WAYLAND_DATASOURCE_H_

#include <set>
#include <wayland-server-protocol.h>

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

 private:
  wl_resource* resource_;
  std::set<DataSourceListener*> listeners_;
};

}  // namespace wayland
}  // namespace naive
#endif  // WAYLAND_DATASOURCE_H_
