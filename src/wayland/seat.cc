#include "wayland/seat.h"

#include "base/logging.h"
#include "wayland/data_device.h"
#include "wayland/data_offer.h"
#include "wayland/data_source.h"
#include "wayland/keyboard.h"

namespace naive {
namespace wayland {

Seat::Seat(
    std::function<DataOffer*(wl_client* client, DataSource* source)> new_offer)
    : data_device_(std::make_unique<DataDevice>(this)), new_offer_(new_offer) {}

void Seat::NotifyKeyboardFocusChanged(Keyboard* to_keyboard) {
  TRACE();
  focused_keyboard_ = to_keyboard;
  OfferSelection();
}

void Seat::NotifyKeyboardDestroyed(Keyboard* keyboard) {
  TRACE();
  if (focused_keyboard_ == keyboard)
    focused_keyboard_ = nullptr;
}

void Seat::OfferSelection() {
  TRACE();
  if (focused_keyboard_ == nullptr)
    return;
  DataSource* selection_source = data_device_->selection();
  if (!selection_source) {
    // TODO: send client a nullptr for no selection.
    return;
  }

  DataOffer* offer =
      new_offer_(wl_resource_get_client(focused_keyboard_->resource()),
                 data_device_->selection());
  auto* device_resource = data_device_->GetBinding(
      wl_resource_get_client(focused_keyboard_->resource()));
  if (!device_resource)
    return;

  wl_data_device_send_data_offer(device_resource, offer->resource());
  for (auto& mime : selection_source->mimetypes())
    wl_data_offer_send_offer(offer->resource(), mime.c_str());
  wl_data_device_send_selection(device_resource, offer->resource());
}

}  // namespace wayland
}  // namespace naive