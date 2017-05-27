CMAKE_MINIMUM_REQUIRED(VERSION 3.6)
PROJECT(NaiveWM CXX)

SET(CMAKE_CXX_COMPILER "clang++")
SET(CMAKE_CXX_FLAGS "-std=c++14 -Werror")

INCLUDE(cmake_modules/FindWayland.cmake)

INCLUDE_DIRECTORIES(${WAYLAND_SERVER_INCLUDE_DIR} src)

FILE(GLOB_RECURSE NAIVE_SOURCE *.cc)

SET(WM_EXECUTABLE naive)
ADD_EXECUTABLE(${WM_EXECUTABLE} ${NAIVE_SOURCE} src/wayland/server.h src/base/macros.h src/wayland/server.cc src/compositor/surface.h src/compositor/buffer.h src/base/geometry.h)
TARGET_LINK_LIBRARIES(${WM_EXECUTABLE} ${WAYLAND_SERVER_LIBRARIES})
