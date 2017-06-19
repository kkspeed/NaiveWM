#ifndef BASE_LOGGING_H_
#define BASE_LOGGING_H_

#include <cstdio>
#include <cstring>
#include <iostream>

#define __FILENAME__ \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define LOG std::cerr

#define LOG_ERROR (std::cerr << "[Error] ")

#define NOTIMPLEMENTED() std::cerr << "Not implemented: " \
                                   << __PRETTY_FUNCTION__ << std::endl;

#ifdef __TRACE__
#define TRACE(format, ...) \
    fprintf(stderr, "[Trace] %s:%d::%s " format "\n",  \
            __FILENAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__);
#else
#define TRACE(...)
#endif

#endif  // BASE_LOGGING_H_
