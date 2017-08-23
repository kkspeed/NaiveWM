#ifndef WAYLAND_TEXT_INPUT_H_
#define WAYLAND_TEXT_INPUT_H_

#include "base/macros.h"

#include <wayland-server-protocol.h>
#include "wayland/keyboard.h"

namespace naive {
namespace wayland {

class TextInput {
 public:
  TextInput();

 private:
  DISALLOW_COPY_AND_ASSIGN(TextInput);
};

class InputMethodContext {
 public:
 private:
  DISALLOW_COPY_AND_ASSIGN(InputMethodContext);
};

class InputMethod {
  class InputMethodActivation {
   public:
    InputMethodActivation(TextInput* text_input, wl_resource* ime_resource);
  };

 public:
  InputMethod(wl_resource* resource);
  void Activate(TextInput* input);

 private:
  Keyboard* keyboard_focus_{nullptr};
  TextInput* input_{nullptr};
  std::unique_ptr<InputMethodActivation> activation_;

  wl_resource* ime_resource_;
  DISALLOW_COPY_AND_ASSIGN(InputMethod);
};

class TextInputManager {
 public:
 private:
  DISALLOW_COPY_AND_ASSIGN(TextInputManager);
};

}  // namespace wayland
}  // namespace naive

#endif  // WAYLAND_TEXT_INPUT_H_
