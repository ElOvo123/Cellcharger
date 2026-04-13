cmake_minimum_required(VERSION 3.16)
project(CellChargerInterface LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS 
    Widgets
    SerialPort
    Network
    Test
)

find_package(yaml-cpp REQUIRED)

option(USE_SIM_COMS "Use simulated communications backend" ON)
option(ENABLE_COVERAGE "Enable compiler coverage instrumentation" OFF)

set(COVERAGE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/coverage-reports")
set(COVERAGE_GCOV_DIR "${COVERAGE_OUTPUT_DIR}/gcov")
set_directory_properties(PROPERTIES
    ADDITIONAL_MAKE_CLEAN_FILES "${COVERAGE_OUTPUT_DIR}"
)

if(USE_SIM_COMS)
    add_compile_definitions(USE_SIM_COMS)
endif()

if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-O0 -g --coverage)
    add_link_options(--coverage)

    add_custom_target(coverage-gcov
        COMMAND ${CMAKE_COMMAND} -E make_directory "${COVERAGE_GCOV_DIR}"
        COMMAND ${CMAKE_COMMAND} -E chdir "${COVERAGE_GCOV_DIR}"
                bash -lc "find '${CMAKE_BINARY_DIR}' -name '*.gcda' -print0 | xargs -0 gcov >/dev/null"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Generating .gcov files in ${COVERAGE_GCOV_DIR}"
    )
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

enable_testing()

configure_file(
    ${CMAKE_SOURCE_DIR}/config/pcp.yaml
    ${CMAKE_BINARY_DIR}/pcp.yaml
    COPYONLY
)

add_executable(qt_app
    main.cpp
)

add_subdirectory(ui)
add_subdirectory(controller)
add_subdirectory(backend)
add_subdirectory(tests)



set_target_properties(qt_app PROPERTIES
    AUTOUIC_SEARCH_PATHS "${CMAKE_SOURCE_DIR}/ui"
)

target_link_libraries(qt_app PRIVATE   
    Qt6::Widgets 
    Qt6::SerialPort 
    Qt6::Network
    yaml-cpp
)
