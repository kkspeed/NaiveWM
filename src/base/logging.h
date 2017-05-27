#ifndef _BASE_LOGGING_H_
#define _BASE_LOGGING_H_

#include <iostream>

#define LOG std::cerr

#define NOTIMPLEMENTED() \
  std::cerr << "Not implemented: " << __PRETTY_FUNCTION__

#endif  // __BASE_LOGGING_H_
