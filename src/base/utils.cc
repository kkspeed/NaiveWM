#include "base/utils.h"

#include <cstdarg>
#include <cstdlib>

namespace naive {
namespace base {

pid_t LaunchProgram(const char* command, char* const argv[]) {
  pid_t pid = fork();
  if (pid != 0)
    return pid;
  if (execvp(command, argv) == -1)
    exit(1);
  return 0;
}

pid_t LaunchProgramV(const char* command, const char* arg, ...) {
  pid_t pid = fork();
  if (pid != 0)
    return pid;
  va_list args;
  va_start(args, arg);
  if (execlp(command, arg, args) == -1)
    exit(1);
  va_end(args);
  return 0;
}

}  // namespace base
}  // namespace naive
