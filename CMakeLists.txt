CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(NaiveWM)

SET(CMAKE_CXX_COMPILER "clang++")
SET(CMAKE_C_COMPILER "clang")
SET(CMAKE_CXX_FLAGS "-D__TRACE__ -D__NAIVE_COMPOSITOR__ -std=c++14 -Werror -ggdb -O0")
SET(CMAKE_EXE_LINKER_FLAGS "-lrt -lSegFault")
SET(CMAKE_C_FLAGS "-Werror")

INCLUDE(cmake_modules/FindEGL.cmake)
INCLUDE(cmake_modules/FindGLESv2.cmake)
INCLUDE(cmake_modules/FindGLM.cmake)
INCLUDE(cmake_modules/FindGBM.cmake)
INCLUDE(cmake_modules/FindLibdrm.cmake)
INCLUDE(cmake_modules/FindLibInput.cmake)
INCLUDE(cmake_modules/FindPixman.cmake)
INCLUDE(cmake_modules/FindUdev.cmake)
INCLUDE(cmake_modules/FindWayland.cmake)
INCLUDE(cmake_modules/FindXKBCommon.cmake)
INCLUDE(cmake_modules/FindGlog.cmake)
INCLUDE(cmake_modules/FindCairomm.cmake)
INCLUDE(cmake_modules/FindDBus.cmake)
INCLUDE(cmake_modules/FindXComposite.cmake)

FIND_PACKAGE(PNG REQUIRED)
FIND_PACKAGE(X11 REQUIRED)
#FIND_PACKAGE(Glog REQUIRED)

INCLUDE_DIRECTORIES(
        ${WAYLAND_SERVER_INCLUDE_DIR}
        ${PIXMAN_INCLUDE_DIRS}
        ${Libdrm_INCLUDE_DIRS}/libdrm
        ${LIBINPUT_INCLUDE_DIRS}
        ${EGL_INCLUDE_DIR}
        ${GLESv2_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
        ${GBM_INCLUDE_DIRS}
        ${UDEV_INCLUDE_DIRS}
        ${XKBCOMMON_INCLUDE_DIRS}
        ${PNG_INCLUDE_DIR}
        ${GLOG_INCLUDE_DIR}
        ${CAIROMM_INCLUDE_DIRS}
        ${DBUS_INCLUDE_DIRS}
        ${X11_INCLUDE_DIR}
        ${XCOMPOSITE_INCLUDE_DIR}
        src
        src/protocols)

FILE(GLOB_RECURSE NAIVE_SOURCE src/*.cc)
FILE(GLOB_RECURSE WAYLAND_EXTRA_PROTOCOLS src/protocols/*.c)

SET(WM_EXECUTABLE naive)
ADD_EXECUTABLE(${WM_EXECUTABLE} 
    ${NAIVE_SOURCE} 
    ${WAYLAND_EXTRA_PROTOCOLS})

TARGET_LINK_LIBRARIES(
        ${WM_EXECUTABLE}
        ${WAYLAND_SERVER_LIBRARIES}
        ${PIXMAN_LIBRARIES}
        ${EGL_LIBRARY}
        ${EGL_opengl_LIBRARY}
        ${GLESv2_LIBRARIES}
        ${Libdrm_LIBRARIES}
        ${LIBINPUT_LIBRARIES}
        ${UDEV_LIBRARIES}
        ${GBM_LIBRARIES}
        ${XKBCOMMON_LIBRARIES}
        ${PNG_LIBRARY}
        ${GLOG_LIBRARY}
        ${CAIROMM_LIBRARIES}
        ${DBUS_LIBRARIES}
        ${X11_LIBRARIES}
        ${XCOMPOSITE_LIBRARY})
