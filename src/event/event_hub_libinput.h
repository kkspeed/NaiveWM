#ifndef EVENT_EVENT_HUB_LIBINPUT_H_
#define EVENT_EVENT_HUB_LIBINPUT_H_

#include <libinput.h>
#include <cstdint>
#include <vector>

#include "event/event_hub.h"

namespace naive {
namespace event {

// establishes a connection between libevent and window manager.
class EventHubLibInput : public EventHub {
 public:
  EventHubLibInput();
  ~EventHubLibInput();

  // EventHub overrides
  int GetFileDescriptor() override;
  void HandleEvents() override;
  void AddEventObserver(EventObserver* observer) override;

 private:
  bool MaybeChangeLockStates(libinput_device* device, uint32_t key);
  bool UpdateModifiers(uint32_t key, libinput_key_state state);
  libinput* libinput_;
  std::vector<EventObserver*> observers_;
  libinput_led leds_ = static_cast<libinput_led>(0);
  uint32_t modifiers_ = 0;
};

}  // namespace event
}  // namespace naive

#endif  // EVENT_EVENTHUB_LIBINPUT_H_
