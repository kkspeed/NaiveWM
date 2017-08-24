#ifndef WAYLAND_TEXT_INPUT_H_
#define WAYLAND_TEXT_INPUT_H_

#include "base/macros.h"

#include <functional>
#include <wayland-server-protocol.h>

#include "wayland/keyboard.h"
#include "compositor/surface.h"
#include "protocols/text-input-unstable-v1.h"
#include "protocols/input-method-unstable-v1.h"
#include "wm/window_manager.h"

namespace naive {

namespace wayland {

class Seat;
class InputMethod;
class InputMethodContext;

class TextInput : public SurfaceObserver {
 public:
  TextInput(wl_resource* resource);
  virtual ~TextInput();
  void SetSurface(Surface* surface);
  void SetInputMethodContext(InputMethodContext* input_method_context);

  // SurfaceObserver overrides
  void OnSurfaceDestroyed(Surface* surface) override;

  void SendEnter();
  void SendLeave();

  InputMethodContext* input_method_context() { return input_method_context_; }

  wl_resource* resource() { return text_input_resource_; }
  Surface* surface() { return surface_; }

 private:
  Surface* surface_{nullptr};
  InputMethodContext* input_method_context_{nullptr};
  wl_resource* text_input_resource_;

  DISALLOW_COPY_AND_ASSIGN(TextInput);
};

class InputMethodContext {
 public:
  InputMethodContext(wl_resource* resource,
                     TextInput* text_input,
                     InputMethod* input_method);
  ~InputMethodContext();

  void Reset();

  void Leave();
  void Enter();
  void NotifyTextInputDestroyed(TextInput* text_input);

  TextInput* text_input() { return text_input_; }
  InputMethod* input_method() { return input_method_; }
  wl_resource* resource() { return context_resource_; }

 private:
  TextInput* text_input_{nullptr};
  wl_resource* context_resource_;
  InputMethod* input_method_;
  DISALLOW_COPY_AND_ASSIGN(InputMethodContext);
};

class InputMethod {
 public:
  InputMethod(Seat* seat);
  void Activate(InputMethodContext* input_method_context);
  void Deactivate();
  void ShowPanels(bool show);

  void SetBinding(wl_resource* resource);
  void NotifyInputContextDestroyed(InputMethodContext* context);
  void NotifyFocusedSurfaceChanged(Surface* surface);

  wl_resource* binding() { return input_method_resource_; }
  Seat* seat() { return seat_; }

 private:
  InputMethodContext* input_method_context_{nullptr};
  wl_resource* input_method_resource_{nullptr};
  Seat* seat_;
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
