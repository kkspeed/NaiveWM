#ifndef BASE_UTILS_H_
#define BASE_UTILS_H_

#include <unistd.h>

namespace naive {
namespace base {

pid_t LaunchProgram(const char* command, char* const argv[]);

}  // namespace base
}  // namespace naive

#endif  // BASE_UTILS_H_
