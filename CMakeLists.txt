cmake_minimum_required(VERSION 2.8)
project(mercpcl)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror")
set(CMAKE_C_FLAGS_PROFILE "-pg")

add_executable(mercpcl mercpcl.c)

if(CMAKE_BUILD_TYPE MATCHES Profile)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ${CMAKE_FIND_LIBRARY_SUFFIXES})
set(STATIC_LIBUSB libusb.a)
find_package(libFTDI REQUIRED)
else(CMAKE_BUILD_TYPE MATCHES Profile)
find_package(libusb-1.0 REQUIRED)
find_package(libFTDI REQUIRED)
set(STATIC_LIBUSB ${LIBUSB-1.0_LIBRARY}) # Not sure what to do w/ Profile yet,
# so just reuse STATIC_LIBUSB until I can improve the Profile build type.
endif(CMAKE_BUILD_TYPE MATCHES Profile)

include_directories(${LIBFTDI_INCLUDE_DIR})
target_link_libraries(mercpcl ${LIBFTDI_LIBRARY} ${STATIC_LIBUSB})

if(MSVC) # Assume MSVC users don't want a bin directory
install(TARGETS mercpcl DESTINATION .)
else(MSVC)
install(TARGETS mercpcl DESTINATION bin)
endif(MSVC)
