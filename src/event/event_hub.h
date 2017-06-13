#ifndef EVENT_EVENT_HUB_H_
#define EVENT_EVENT_HUB_H_

extern "C" {
struct libinput;
};

namespace naive {
namespace event {

// establishes a connection between libevent and window manager.
class EventHub {
 public:
  static void InitializeEventHub();
  static EventHub* Get();

  EventHub();
  ~EventHub();

  int GetFileDescriptor();
  void HandleEvents();

 private:
  static EventHub* g_event_hub;
  libinput* libinput_;
};

}  // namespace event
}  // namespace naive

#endif  // EVENT_EVENTHUB_H_
