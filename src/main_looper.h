#ifndef MAIN_LOOPER_H_
#define MAIN_LOOPER_H_

#include <functional>
#include <poll.h>
#include <vector>

namespace naive {

using HandlerFunc = std::function<void()>;

class MainLooper {
 public:
  MainLooper() = default;
  static MainLooper* Get();

  void AddFd(int fd, HandlerFunc handler);
  void AddHandler(HandlerFunc handler);

  void Run();

 private:
  std::vector<pollfd> fds_;
  std::vector<HandlerFunc> handlers_;
};

}  // namespace naive

#endif  // MAIN_LOOPER_H_