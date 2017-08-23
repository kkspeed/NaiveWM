#include "wayland/text_input.h"

namespace naive {
namespace wayland {

TextInput::TextInput() {}

InputMethod::InputMethod(wl_resource* resource) : ime_resource_{resource} {}

void InputMethod::Activate(TextInput* text_input) {
  activation_ = std::make_unique<InputMethodActivation>(text_input, resource);
  text_input_ = text_input;
}

InputMethod::InputMethodActivation::InputMethodActivation(
    TextInput* text_input,
    wl_resource* ime_resource) {}

}  // namespace wayland
}  // namespace naive
