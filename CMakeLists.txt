cmake_minimum_required(VERSION 3.10)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" THE_VERSION)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to MinSizeRel")
    set(CMAKE_BUILD_TYPE "MinSizeRel" CACHE STRING "Build type")
endif()

project("simonedegiacomi/EV3-API"
        VERSION ${THE_VERSION}
        LANGUAGES C
        DESCRIPTION "EV3-API for Programming the LEGO Mindstorms EV3 in C")

# add compiler options
add_compile_options(-fno-strict-aliasing -fwrapv)
add_compile_options(-Wall -Wextra -Wpointer-sign -Wno-unused-parameter)
add_compile_options(-fdata-sections -ffunction-sections)
add_compile_options(-Wl,--gc-sections)
add_definitions(-DC4EV3_VERSION=${PROJECT_VERSION})

########################################################################
# LIBRARY

# allow deactivation of timers
option(DISABLE_TIMERS "Disable timers" OFF)
set_property(CACHE DISABLE_TIMERS
             PROPERTY HELPSTRING
             "Disable the ev3_timer module. This will prevent periodic syscall interruptions caused by the SIGALRM signal.")

# add sources
FILE(GLOB_RECURSE EV3API_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/firmware_headers/*.c)
FILE(GLOB_RECURSE EV3API_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/firmware_headers/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
FILE(GLOB_RECURSE EV3API_BITMAPS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.xbm)

# define library
add_library(ev3api STATIC ${EV3API_SOURCES} ${EV3API_HEADERS} ${EV3API_BITMAPS})
add_library(c4ev3::ev3api ALIAS ev3api)

# and its include directories
target_include_directories(ev3api
        PUBLIC
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src)
# and its macros
if (DISABLE_TIMERS)
    target_compile_definitions(c4ev3::ev3api PUBLIC -DDISABLE_TIMERS=1)
endif()

########################################################################
# EXAMPLE TEST FILE

# also add a minimal test file
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/example.c "#include <ev3.h>\r\nint main(void) { return EV3IsInitialized() == 1; }\r\n")

add_executable(example.elf EXCLUDE_FROM_ALL ${CMAKE_CURRENT_BINARY_DIR}/example.c)
target_link_libraries(example.elf PRIVATE c4ev3::ev3api)

########################################################################
# REAL EXAMPLES

option(ENABLE_EXAMPLES "Enable examples" ON)
if (ENABLE_EXAMPLES)
    add_subdirectory(examples EXCLUDE_FROM_ALL)
endif()

########################################################################
# DOCUMENTATION

option(ENABLE_DOCS "Enable documentation" ON)
if (ENABLE_DOCS)
    find_package(Doxygen COMPONENTS dot)
    if (DOXYGEN_FOUND)
        set(DOXYGEN_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/include")
        set(DOXYGEN_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/docs")
        set(DOXYFILE_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in")
        set(DOXYFILE          "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")

        configure_file("${DOXYFILE_TEMPLATE}" "${DOXYFILE}")

        add_custom_target(docs VERBATIM
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DOXYGEN_OUTPUT}"
            COMMAND Doxygen::doxygen "${DOXYFILE}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            DEPENDS "${DOXYFILE}"
            COMMENT "Generating Doxygen documentation"
            ${EV3API_HEADERS}
        )
    endif()
endif()
