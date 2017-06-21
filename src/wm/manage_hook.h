#ifndef MANAGE_HOOK_H_
#define MANAGE_HOOK_H_

#include <cstdint>

#include "wm/window_manager.h"

namespace naive {
namespace wm {

class Window;
class WMPrimitives;
class Event;
class MouseEvent;

class ManageHook : public WmEventObserver {
 public:
  void WindowCreated(Window* window) override;
  void WindowDestroying(Window* window) override;
  void WindowDestroyed(Window* window) override;
  bool OnMouseEvent(MouseEvent* event) override;
  bool OnKey(KeyboardEvent* event) override;

  void set_wm_primitives(WMPrimitives* primitives) override {
    primitives_ = primitives;
  }

 private:
  int32_t width_ = 1280, height_ = 720;
  WMPrimitives* primitives_ = nullptr;
};

}  // wm
}  // naive

#endif  // MANAGE_HOOK_H_
