#include "wayland/text_input.h"

namespace naive {
namespace wayland {

TextInput::TextInput(wl_resource* resource) : text_input_resource_{resource} {}

void TextInput::SetSurface(Surface* surface) {
  if (surface_ == surface)
    return;
  if (surface_)
    surface_->RemoveSurfaceObserver(this);
  surface_ = surface;
  if (!surface_)
    return;
  surface->AddSurfaceObserver(this);
}

TextInput::~TextInput() {
  if (surface_)
    surface_->RemoveSurfaceObserver(this);
  if (input_method_context_)
    input_method_context_->NotifyTextInputDestroyed(this);
}

void TextInput::SetInputMethodContext(
    InputMethodContext* input_method_context) {
  input_method_context_ = input_method_context;
}

void TextInput::SendEnter() {
  zwp_text_input_v1_send_enter(text_input_resource_, surface_->resource());
}

void TextInput::SendLeave() {
  zwp_text_input_v1_send_leave(text_input_resource_);
}

void TextInput::OnSurfaceDestroyed(Surface* surface) {
  if (surface == surface_)
    surface_ = nullptr;
}

InputMethodContext::InputMethodContext(wl_resource* resource,
                                       TextInput* text_input,
                                       InputMethod* input_method)
    : text_input_{text_input},
      context_resource_{resource},
      input_method_{input_method} {}

InputMethodContext::~InputMethodContext() {
  TRACE();
  input_method_->NotifyInputContextDestroyed(this);
  if (text_input_ && text_input_->input_method_context() == this) {
    text_input_->SetInputMethodContext(nullptr);
    text_input_->SendLeave();
  }
}

void InputMethodContext::Reset() {
  zwp_input_method_context_v1_send_reset(context_resource_);
}

void InputMethodContext::Leave() {
  if (text_input_)
    text_input_->SendLeave();
}

void InputMethodContext::Enter() {
  if (text_input_)
    text_input_->SendEnter();
}

void InputMethodContext::NotifyTextInputDestroyed(TextInput* text_input) {
  if (text_input == text_input_)
    text_input_ = nullptr;
}

InputMethod::InputMethod(Seat* seat) : seat_{seat} {}

void InputMethod::ShowPanels(bool show) {
  auto* top_level = wm::WindowManager::Get()->input_panel_top_level();
  auto* overlay = wm::WindowManager::Get()->input_panel_overlay();
  if (top_level && overlay) {
    overlay->set_visible(show);
    top_level->set_visible(show);
  }
}

void InputMethod::Activate(InputMethodContext* context) {
  TRACE();
  if (context == input_method_context_)
    return;
  if (input_method_context_)
    input_method_context_->Leave();
  input_method_context_ = context;
  if (!context)
    return;
  zwp_input_method_v1_send_activate(input_method_resource_,
                                    context->resource());
  context->Enter();
}

void InputMethod::Deactivate() {
  if (input_method_context_) {
    input_method_context_->Leave();
    zwp_input_method_v1_send_deactivate(input_method_resource_,
                                        input_method_context_->resource());
  }
}

void InputMethod::NotifyInputContextDestroyed(InputMethodContext* context) {
  if (context == input_method_context_)
    input_method_context_ = nullptr;
}

void InputMethod::NotifyFocusedSurfaceChanged(Surface* surface) {
  if (input_method_context_ && input_method_context_->text_input() &&
      input_method_context_->text_input()->surface() != surface) {
    Deactivate();
  }
}

void InputMethod::SetBinding(wl_resource* resource) {
  TRACE("Binding %p", resource);
  input_method_resource_ = resource;
}

}  // namespace wayland
}  // namespace naive
