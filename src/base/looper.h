#ifndef BASE_LOOPER_H_
#define BASE_LOOPER_H_

#include <functional>

namespace naive {
namespace base {

using HandlerFunc = std::function<void()>;

class Looper {
 public:
  virtual void AddFd(int fd, HandlerFunc func) = 0;
  virtual void AddHandler(HandlerFunc func) = 0;
};

}  // namespace base
}  // namespace naive

#endif  // BASE_LOOPER_H_
