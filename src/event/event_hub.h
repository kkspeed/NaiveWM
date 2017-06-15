#ifndef EVENT_EVENT_HUB_H_
#define EVENT_EVENT_HUB_H_

#include <cstdint>
#include <vector>

extern "C" {
struct libinput;
};

namespace naive {
namespace event {

class EventObserver {
 public:
  virtual void OnMouseButton(uint32_t button, bool down) = 0;
  virtual void OnMouseMotion(float dx, float dy) = 0;
};

// establishes a connection between libevent and window manager.
class EventHub {
 public:
  static void InitializeEventHub();
  static EventHub* Get();

  EventHub();
  ~EventHub();

  int GetFileDescriptor();
  void HandleEvents();

  void AddEventObserver(EventObserver* observer);

 private:
  static EventHub* g_event_hub;
  libinput* libinput_;
  std::vector<EventObserver*> observers_;
};

}  // namespace event
}  // namespace naive

#endif  // EVENT_EVENTHUB_H_
