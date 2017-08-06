# Try to find Cairomm
# Released under MIT License by Kota Weaver
# Once done this will define
#  CAIROMM_INCLUDE_DIRS - The Cairomm include directories
#  CAIROMM_LIBRARIES - The libraries needed to use Cairomm
#  CAIROMM_DEFINITIONS - Compiler switches required for using Cairomm

find_package(PkgConfig)
pkg_search_module(Cairo REQUIRED cairo)
pkg_search_module(SigC++ REQUIRED sigc++-2.0)

find_path(CAIROMM_INCLUDE_DIR cairomm/cairomm.h
  HINTS /usr/include/cairomm-1.0)
find_library(CAIROMM_LIBRARY NAMES cairomm-1.0)
mark_as_advanced(CAIROMM_INCLUDE_DIR CAIROMM_LIBRARY)
set(CAIROMM_LIBRARIES ${CAIROMM_LIBRARY} ${SigC++_LIBRARIES} ${Cairo_LIBRARIES})
set(CAIROMM_INCLUDE_DIRS ${CAIROMM_INCLUDE_DIR} ${SigC++_INCLUDE_DIRS} ${Cairo_INCLUDE_DIRS})

