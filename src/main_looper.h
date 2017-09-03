#ifndef MAIN_LOOPER_H_
#define MAIN_LOOPER_H_

#include <poll.h>
#include <functional>
#include <vector>

#include "base/looper.h"

namespace naive {

class MainLooper : public base::Looper {
 public:
  MainLooper() = default;
  static MainLooper* Get();

  // base::Looper overrides.
  void AddFd(int fd, base::HandlerFunc handler) override;
  void AddHandler(base::HandlerFunc handler) override;

  void Run();

 private:
  std::vector<pollfd> fds_;
  std::vector<base::HandlerFunc> handlers_;
};

}  // namespace naive

#endif  // MAIN_LOOPER_H_
