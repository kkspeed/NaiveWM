CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(NaiveWM)

SET(CMAKE_CXX_COMPILER "clang++")
SET(CMAKE_CXX_FLAGS "-std=c++14 -Werror")
SET(CMAKE_C_COMPILER "clang")
SET(CMAKE_C_FLAGS "-Werror")

INCLUDE(cmake_modules/FindWayland.cmake)
INCLUDE(cmake_modules/FindPixman.cmake)
INCLUDE(cmake_modules/FindLibdrm.cmake)
INCLUDE(cmake_modules/FindEGL.cmake)
INCLUDE(cmake_modules/FindGLESv2.cmake)
INCLUDE(cmake_modules/FindGBM.cmake)

INCLUDE_DIRECTORIES(
        ${WAYLAND_SERVER_INCLUDE_DIR}
        ${PIXMAN_INCLUDE_DIRS}
        ${Libdrm_INCLUDE_DIRS}/libdrm
        ${EGL_INCLUDE_DIR}
        ${GLESv2_INCLUDE_DIRS}
        ${GBM_INCLUDE_DIRS}
        src)

FILE(GLOB_RECURSE NAIVE_SOURCE src/*.cc)
FILE(GLOB_RECURSE WAYLAND_EXTRA_PROTOCOLS src/*.c)

SET(WM_EXECUTABLE naive)
ADD_EXECUTABLE(${WM_EXECUTABLE} ${NAIVE_SOURCE} ${WAYLAND_EXTRA_PROTOCOLS})
TARGET_LINK_LIBRARIES(
        ${WM_EXECUTABLE}
        ${WAYLAND_SERVER_LIBRARIES}
        ${PIXMAN_LIBRARIES}
        ${EGL_LIBRARY}
        ${EGL_opengl_LIBRARY}
        ${GLESv2_LIBRARIES}
        ${Libdrm_LIBRARIES}
        ${GBM_LIBRARIES})
