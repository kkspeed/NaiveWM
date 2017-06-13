#.rst:
# FindUdev
# -------
#
# Find Udev library
#
# Try to find Udev library on UNIX systems. The following values are defined
#
# ::
#
#   UDEV_FOUND         - True if udev is available
#   UDEV_INCLUDE_DIRS  - Include directories for udev
#   UDEV_LIBRARIES     - List of libraries for udev
#
#=============================================================================
# Copyright (c) 2017 Bruce Li
#
# Distributed under the OSI-approved BSD License (the "License");
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================

find_package(PkgConfig)
pkg_check_modules(PC_UDEV QUIET udev)
find_library(UDEV_LIBRARIES NAMES udev HINTS ${PC_UDEV_LIBRARY_DIRS})
find_path(UDEV_INCLUDE_DIRS libudev.h HINTS ${PC_UDEV_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UDEV DEFAULT_MSG UDEV_INCLUDE_DIRS UDEV_LIBRARIES)
mark_as_advanced(UDEV_INCLUDE_DIRS UDEV_LIBRARIES)

