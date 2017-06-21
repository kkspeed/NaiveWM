#include "base/utils.h"

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

}  // namespace base
}  // namespace naive
