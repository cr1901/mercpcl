cmake_minimum_required(VERSION 3.18)

project(mercpcl)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror")
set(CMAKE_C_FLAGS_PROFILE "-pg")

add_executable(mercpcl mercpcl.c)

find_package(PkgConfig)

if(${PKG_CONFIG_FOUND})
    pkg_check_modules(mercpl_deps REQUIRED libusb libftdi1)
    pkg_check_modules(libusb REQUIRED libusb)
endif()

if(${mercpl_deps_FOUND})
    target_include_directories(mercpcl PRIVATE ${mercpl_deps_INCLUDE_DIRS})
    target_link_libraries(mercpcl ${mercpl_deps_LINK_LIBRARIES})
else()
    find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h REQUIRED PATH_SUFFIXES libusb-1.0)
    find_path(LIBFTDI1_INCLUDE_DIR NAMES ftdi.h REQUIRED PATH_SUFFIXES libftdi1)

    find_library(LIBUSB_LIBRARY libusb-1.0 REQUIRED)
    find_library(LIBFTDI1_LIBRARY libftdi1 REQUIRED)

    target_include_directories(mercpcl PRIVATE ${LIBUSB_INCLUDE_DIR} ${LIBFTDI1_INCLUDE_DIR})
    target_link_libraries(mercpcl  ${LIBFTDI1_LIBRARY} ${LIBUSB_LIBRARY})
endif()

install(TARGETS mercpcl DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
