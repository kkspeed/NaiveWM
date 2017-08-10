#ifndef BASE_LOGGING_H_
#define BASE_LOGGING_H_

#include <glog/logging.h>
#include <cstdio>
#include <cstring>
#include <iostream>

#define __FILENAME__ \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define LOG_ERROR LOG(ERROR)
#define LOG_INFO LOG(INFO)
#define LOG_FATAL LOG(FATAL)

#ifdef __TRACE__

#define NOTIMPLEMENTED() \
  LOG(ERROR) << "Not implemented: " << __PRETTY_FUNCTION__ << std::endl;

#define TRACE(format, ...)                                                  \
  fprintf(stderr, "[Trace] %s:%d::%s " format "\n", __FILENAME__, __LINE__, \
          __FUNCTION__, ##__VA_ARGS__);
#else
#define TRACE(...)
#define NOTIMPLEMENTED()
#endif

#endif  // BASE_LOGGING_H_
