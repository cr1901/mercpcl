cmake_minimum_required(VERSION 3.18)

project(mercpcl)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror")
set(CMAKE_C_FLAGS_PROFILE "-pg")

option(COPY_DLLS "Copy non-system DLLs to install directory (windows only)" NO)

add_executable(mercpcl mercpcl.c)

function (find_header NAME HEADER_NAME SUFFIX)
    string(TOUPPER ${NAME} UPPER_NAME)
    message(CHECK_START "Finding ${NAME} include directory")
    find_path(${UPPER_NAME}_INCLUDE_DIR NAMES ${HEADER_NAME} REQUIRED PATH_SUFFIXES ${SUFFIX})
    message(CHECK_PASS "found at ${${UPPER_NAME}_INCLUDE_DIR}")
endfunction()

find_header(libusb libusb.h libusb-1.0)
find_header(libftdi1 ftdi.h libftdi1)

function (find_lib NAME)
    string(TOUPPER ${NAME} UPPER_NAME)
    message(CHECK_START "Finding ${NAME} library")

    add_library(${NAME} SHARED IMPORTED)
    if(WIN32)
        find_library(${UPPER_NAME}_LIBRARY ${NAME} REQUIRED)
        find_file(${UPPER_NAME}_DLL ${NAME}.dll REQUIRED)
        # https://discourse.cmake.org/t/find-library-wont-find-dlls/4050/3
        set_target_properties(${NAME} PROPERTIES
            IMPORTED_LOCATION ${${UPPER_NAME}_DLL}
            IMPORTED_IMPLIB ${${UPPER_NAME}_LIBRARY}
        )
    else()
        find_library(${UPPER_NAME}_LIBRARY ${NAME} REQUIRED)
        set_target_properties(${NAME} PROPERTIES
            IMPORTED_LOCATION ${${UPPER_NAME}_LIBRARY}
        )
    endif()
    message(CHECK_PASS "found at ${${UPPER_NAME}_LIBRARY}")
endfunction()

find_lib(libusb-1.0)
find_lib(libftdi1)

target_include_directories(mercpcl PRIVATE ${LIBUSB_INCLUDE_DIR} ${LIBFTDI1_INCLUDE_DIR})
target_link_libraries(mercpcl libftdi1 libusb-1.0)

install(TARGETS mercpcl DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# As a courtesy, on Windows copy over DLLs to our install directory. This is
# best-effort and will only find DLLs in the same directories as our immediate
# dependencies.
if(WIN32 AND COPY_DLLS)
    install( CODE [[
        foreach(D $<TARGET_PROPERTY:libftdi1,IMPORTED_LOCATION> $<TARGET_PROPERTY:libusb-1.0,IMPORTED_LOCATION>)
            get_filename_component(DLL_DIR ${D} DIRECTORY)
            list(APPEND DLL_DIRS ${DLL_DIR})
        endforeach()

        file(GET_RUNTIME_DEPENDENCIES
            EXECUTABLES $<TARGET_FILE:mercpcl>
            RESOLVED_DEPENDENCIES_VAR R_DEPS
            # https://discourse.cmake.org/t/get-runtime-dependencies-has-difficulties-with-windows-api-sets/1768
            PRE_EXCLUDE_REGEXES "api-ms-win-.*\\.dll"
            DIRECTORIES ${DLL_DIRS}
        )

        # https://gist.github.com/micahsnyder/5d98ac8548b429309ec5a35bca9366da
        foreach(F ${R_DEPS})
            string(TOLOWER ${F} F_LOWER)
            if(NOT ${F_LOWER} MATCHES "c:[\\/]windows[\\/]system32.*")
                file(COPY ${F} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
            endif()
        endforeach()
    ]])
endif()
