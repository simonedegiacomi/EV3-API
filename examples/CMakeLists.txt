cmake_minimum_required(VERSION 3.10)

project("C4EV3 Examples"
        VERSION 0.1.0
        DESCRIPTION "EV3-API example code"
        LANGUAGES C)

add_custom_target(examples)

FILE(GLOB_RECURSE EXAMPLE_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

foreach(EXAMPLE_SRC IN LISTS EXAMPLE_SRCS)
    get_filename_component(EXAMPLE_TARGET ${EXAMPLE_SRC} NAME_WE)
    set(EXAMPLE_EXE "${EXAMPLE_TARGET}.elf")

    add_executable(${EXAMPLE_TARGET}
                   ${EXAMPLE_SRC})

    set_target_properties(${EXAMPLE_TARGET}
                          PROPERTIES OUTPUT_NAME ${EXAMPLE_EXE})

    target_link_libraries(${EXAMPLE_TARGET} c4ev3::ev3api)
    add_dependencies(examples ${EXAMPLE_TARGET})
endforeach()
