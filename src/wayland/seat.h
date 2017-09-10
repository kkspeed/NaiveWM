#ifndef WAYLAND_SEAT_H_
#define WAYLAND_SEAT_H_

#include <functional>
#include <map>
#include <memory>

#include "wayland/data_device.h"
#include "wayland/keyboard.h"
#include "wayland/text_input.h"

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
  void RegisterKeyboard(wl_client* client, Keyboard* keyboard);
  void RemoveKeyboard(wl_client*);

  DataDevice* data_device() { return data_device_.get(); }
  InputMethod* input_method() { return &input_method_; }
  Keyboard* focused_keyboard() { return focused_keyboard_; }

  Keyboard* GetKeyboardByClient(wl_client* client) {
    auto iter = keyboard_bindings_.find(client);
    return iter == keyboard_bindings_.end() ? nullptr : (*iter).second;
  }

 private:
  std::unique_ptr<DataDevice> data_device_;
  std::function<DataOffer*(wl_client* client, DataSource* source)> new_offer_;
  std::map<wl_client*, Keyboard*> keyboard_bindings_;
  Keyboard* focused_keyboard_{nullptr};
  InputMethod input_method_;
};

}  // namespace wayland
}  // namespace wayland

#endif  // WAYLAND_SEAT_H_
