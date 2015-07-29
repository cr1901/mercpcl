cmake_minimum_required(VERSION 2.8)
project(mercpcl)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

add_executable(mercpcl mercpcl.c)

find_package(LIBFTDI REQUIRED)
include_directories(${LIBFTDI_INCLUDE_DIR})
target_link_libraries(mercpcl ${LIBFTDI_LIBRARY})

if(MSVC) # Assume MSVC users don't want a bin directory 
install(TARGETS mercpcl DESTINATION .)
else(MSVC)
install(TARGETS mercpcl DESTINATION bin)
endif(MSVC)

