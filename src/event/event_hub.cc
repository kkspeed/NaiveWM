#include "event_hub.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <libinput.h>
#include <unistd.h>

#include "base/logging.h"

namespace naive {
namespace event {

namespace {

int open_restricted(const char* path, int flags, void* user_data) {
  int fd = open(path, flags);

  if (fd < 0)
    fprintf(stderr, "Failed to open %s (%s)\n", path, strerror(errno));

  return fd < 0 ? -errno : fd;
}

void close_restricted(int fd, void* user_data) {
  close(fd);
}

const struct libinput_interface interface = {
    .close_restricted = close_restricted,
    .open_restricted = open_restricted
};

libinput* open_libinput_udev(const struct libinput_interface* interface,
                             const char* seat) {
  libinput* li;
  udev* udev = udev_new();
  assert(udev);
  li = libinput_udev_create_context(interface, nullptr, udev);
  assert(li);
  assert(!libinput_udev_assign_seat(li, seat));
  udev_unref(udev);
  return li;
}

}  // namespace

// static
EventHub* EventHub::g_event_hub = nullptr;

// static
void EventHub::InitializeEventHub() {
  assert(!g_event_hub);
  g_event_hub = new EventHub();
}

// static
EventHub* EventHub::Get() {
  assert(g_event_hub);
  return g_event_hub;
}

EventHub::EventHub() {
  libinput_ = open_libinput_udev(&interface, "seat0");
  HandleEvents();
}

EventHub::~EventHub() {
  libinput_unref(libinput_);
}

void EventHub::AddEventObserver(EventObserver* observer) {
  observers_.push_back(observer);
}

int EventHub::GetFileDescriptor() {
  return libinput_get_fd(libinput_);
}

void EventHub::HandleEvents() {
  libinput_event* ev;
  libinput_dispatch(libinput_);
  while ((ev = libinput_get_event(libinput_))) {
    switch (libinput_event_get_type(ev)) {
      case LIBINPUT_EVENT_NONE:
        LOG_ERROR << "can't figure out event type" << std::endl;
      case LIBINPUT_EVENT_DEVICE_ADDED:
      case LIBINPUT_EVENT_DEVICE_REMOVED:
        LOG_ERROR << "device changed" << std::endl;
        break;
      case LIBINPUT_EVENT_KEYBOARD_KEY:
        LOG_ERROR << "keyboard key" << std::endl;
        break;
      case LIBINPUT_EVENT_POINTER_MOTION: {
        LOG_ERROR << "pointer motion" << std::endl;
      	libinput_event_pointer *p = libinput_event_get_pointer_event(ev);
        float x = libinput_event_pointer_get_dx(p);
        float y = libinput_event_pointer_get_dy(p);
        for (auto* observer: observers_)
          observer->OnMouseMotion(x, y);
        break;
      }
      case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
        LOG_ERROR << "pointer motion absolute" << std::endl;
        break;
      case LIBINPUT_EVENT_POINTER_BUTTON:
        LOG_ERROR << "pointer motion button" << std::endl;
        break;
      case LIBINPUT_EVENT_POINTER_AXIS:
        LOG_ERROR << "pointer motion axis" << std::endl;
        break;
      default:
        break;
    }

    libinput_event_destroy(ev);
    libinput_dispatch(libinput_);
  }
}

}  // namespace event
}  // namespace naive
