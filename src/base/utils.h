#ifndef BASE_UTILS_H_
#define BASE_UTILS_H_

#include <unistd.h>

namespace naive {
namespace base {

pid_t LaunchProgram(const char* command, char* const argv[]);
pid_t LaunchProgramV(const char* command, const char* arg, ...);

}  // namespace base
}  // namespace naive

#endif  // BASE_UTILS_H_
