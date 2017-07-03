#ifndef WAYLAND_SEAT_H_
#define WAYLAND_SEAT_H_

#include <functional>
#include <memory>

#include "wayland/data_device.h"

namespace naive {
namespace wayland {

class Keyboard;
class DataDevice;
class DataOffer;

class Seat {
 public:
  explicit Seat(std::function<DataOffer*(wl_client* client, DataSource* source)>
                    new_offer);

  void NotifyKeyboardFocusChanged(Keyboard* to_keyboard);
  void NotifyKeyboardDestroyed(Keyboard* keyboard);
  void OfferSelection();

  DataDevice* data_device() { return data_device_.get(); }

 private:
  std::unique_ptr<DataDevice> data_device_;
  std::function<DataOffer*(wl_client* client, DataSource* source)> new_offer_;
  Keyboard* focused_keyboard_;
};

}  // namespace wayland
}  // namespace wayland

#endif  // WAYLAND_SEAT_H_
