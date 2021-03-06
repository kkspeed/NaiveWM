#include "wayland/seat.h"

#include "base/logging.h"
#include "wayland/data_device.h"
#include "wayland/data_offer.h"
#include "wayland/data_source.h"
#include "wayland/text_input.h"

namespace naive {
namespace wayland {

Seat::Seat(
    std::function<DataOffer*(wl_client* client, DataSource* source)> new_offer)
    : data_device_(std::make_unique<DataDevice>(this)),
      new_offer_(new_offer),
      input_method_(this) {}

void Seat::RegisterKeyboard(wl_client* client, Keyboard* keyboard) {
  if (keyboard_bindings_.find(client) != keyboard_bindings_.end())
    TRACE(
        "Keyboard already exists.. not one client per keyboard? client: %p, "
        "keyboard: %p",
        client, keyboard);
  keyboard_bindings_[client] = keyboard;
}

void Seat::RemoveKeyboard(wl_client* client) {
  keyboard_bindings_.erase(client);
  TRACE("client: %p, now has %lu keyboards", client, keyboard_bindings_.size());
}

void Seat::NotifyKeyboardFocusChanged(Keyboard* to_keyboard) {
  TRACE("focused keyboard: %p, to_keyboard: %p", focused_keyboard_,
        to_keyboard);
  if (focused_keyboard_ != to_keyboard) {
    if (focused_keyboard_) {
      focused_keyboard_->Grab(nullptr);
      input_method_.NotifyFocusedSurfaceChanged(
          to_keyboard ? to_keyboard->target_surface() : nullptr);
    }
    focused_keyboard_ = to_keyboard;
    OfferSelection();
  }
}

void Seat::NotifyKeyboardDestroyed(Keyboard* keyboard) {
  TRACE();
  if (focused_keyboard_ == keyboard)
    focused_keyboard_ = nullptr;
  if (keyboard)
    RemoveKeyboard(wl_resource_get_client(keyboard->resource()));
}

void Seat::OfferSelection() {
  TRACE();
  if (focused_keyboard_ == nullptr)
    return;
  DataSource* selection_source = data_device_->selection();
  auto* device_resource = data_device_->GetBinding(
      wl_resource_get_client(focused_keyboard_->resource()));

  if (!device_resource)
    return;

  if (!selection_source) {
    wl_data_device_send_selection(device_resource, nullptr);
    return;
  }

  TRACE("send offer to %p", device_resource);
  DataOffer* offer =
      new_offer_(wl_resource_get_client(focused_keyboard_->resource()),
                 data_device_->selection());

  wl_data_device_send_data_offer(device_resource, offer->resource());
  for (auto& mime : selection_source->mimetypes())
    wl_data_offer_send_offer(offer->resource(), mime.c_str());
  wl_data_device_send_selection(device_resource, offer->resource());
}

}  // namespace wayland
}  // namespace naive
