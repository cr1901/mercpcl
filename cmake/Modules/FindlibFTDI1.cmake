# - Try to find libftdi
# Once done, this will define
#
#  LIBFTDI_FOUND - system has LIBFTDI
#  LIBFTDI_INCLUDE_DIRS - the LIBFTDI include directories
#  LIBFTDI_LIBRARIES - link these to use LIBFTDI

# Inspired by: https://github.com/psi46/pxar/blob/master/cmake/FindFTD2XX.cmake
include(LibFindMacros)

# Include dir
find_path(LIBFTDI1_INCLUDE_DIR
  NAMES libftdi1/ftdi.h
)

# Finally the library itself
find_library(LIBFTDI1_LIBRARY
  NAMES ftdi ftdi1
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
libfind_process(LIBFTDI1)
