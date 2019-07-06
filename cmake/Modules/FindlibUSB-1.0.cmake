# - Try to find libusb-1.0
# Once done, this will define
#
#  LIBUSB-1.0_FOUND - system has LIBUSB-1.0
#  LIBUSB-1.0_INCLUDE_DIRS - the LIBUSB-1.0 include directories
#  LIBUSB-1.0_LIBRARIES - link these to use LIBUSB-1.0

include(LibFindMacros)

# Include dir
find_path(LIBUSB-1.0_INCLUDE_DIR
  NAMES libusb-1.0/libusb.h
)

# Finally the library itself
find_library(LIBUSB-1.0_LIBRARY
  NAMES usb-1.0
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
libfind_process(LIBUSB-1.0)
