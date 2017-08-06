#ifndef WAYLAND_SEAT_H_
#define WAYLAND_SEAT_H_

#include <functional>
#include <map>
#include <memory>

#include "wayland/data_device.h"
#include "wayland/keyboard.h"

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
  void RegisterKeyboard(wl_client* client, std::unique_ptr<Keyboard> keyboard);
  void RemoveKeyboard(wl_client*);

  DataDevice* data_device() { return data_device_.get(); }

 private:
  std::unique_ptr<DataDevice> data_device_;
  std::function<DataOffer*(wl_client* client, DataSource* source)> new_offer_;
  std::map<wl_client*, std::unique_ptr<Keyboard>> keyboard_bindings_;
  Keyboard* focused_keyboard_;
};

}  // namespace wayland
}  // namespace wayland

#endif  // WAYLAND_SEAT_H_
