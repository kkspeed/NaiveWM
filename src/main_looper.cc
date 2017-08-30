#include "main_looper.h"

#include <algorithm>

namespace naive {

static MainLooper* g_looper = nullptr;

// static
MainLooper* MainLooper::Get() {
  if (!g_looper)
    g_looper = new MainLooper();
  return g_looper;
}

void MainLooper::AddFd(int fd, HandlerFunc handler) {
  if (std::find_if(fds_.begin(), fds_.end(),
                   [fd](pollfd& pfd) { return pfd.fd == fd; }) != fds_.end())
    return;

  fds_.push_back({.fd = fd, .events = POLLIN});
  handlers_.push_back(handler);
}

void MainLooper::AddHandler(HandlerFunc handler) {
  handlers_.push_back(handler);
}

void MainLooper::Run() {
  for (;;) {
    for (size_t i = 0; i < handlers_.size(); i++)
      handlers_[i]();
    poll(fds_.data(), fds_.size(), 1);
  }
}

}  // namespace naive