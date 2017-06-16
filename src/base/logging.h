#ifndef _BASE_LOGGING_H_
#define _BASE_LOGGING_H_

#include <cstdio>
#include <iostream>

#define LOG std::cerr

#define LOG_ERROR (std::cerr << "[Error] ")

#define NOTIMPLEMENTED() std::cerr << "Not implemented: " \
                                   << __PRETTY_FUNCTION__ << std::endl;

#ifdef __TRACE__
#define TRACE(format, ...) \
    fprintf(stderr, "[Trace] %s " format "\n",  \
            __FUNCTION__, ##__VA_ARGS__);
#else
#define TRACE()
#endif

#endif  // __BASE_LOGGING_H_
