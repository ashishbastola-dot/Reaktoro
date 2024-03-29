file(GLOB_RECURSE CPPFILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

include_directories(${PROJECT_SOURCE_DIR})

# The global path to the examples directory (used below as a compile definition for the C++ examples)
set(REAKTORO_EXAMPLES_DIR ${PROJECT_SOURCE_DIR}/examples)

foreach(CPPFILE ${CPPFILES})
    string(FIND "${CPPFILE}" "rkp.cpp" result)
    if(${result} GREATER 0 AND NOT reaktplot_FOUND)
        message(STATUS "Example ${CPPFILE} will not be compiled because it depends on reaktplot, which was not found.")
        continue()
    endif()

    get_filename_component(CPPNAME ${CPPFILE} NAME_WE)
    add_executable(${CPPNAME} ${CPPFILE})
    target_link_libraries(${CPPNAME} Reaktoro::Reaktoro)
    target_compile_definitions(${CPPNAME} PRIVATE REAKTORO_EXAMPLES_DIR="${REAKTORO_EXAMPLES_DIR}")  # This permits the C++ examples to load resource files using global paths so that the tests can be executed from anywhere without errors.

    if(${result} GREATER 0)
        target_link_libraries(${CPPNAME} reaktplot::reaktplot)
    endif()

endforeach()
