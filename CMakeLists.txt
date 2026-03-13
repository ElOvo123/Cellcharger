cmake_minimum_required(VERSION 3.16)
project(CellChargerInterface LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS 
    Widgets
    SerialPort
    Network
)

option(USE_SIM_COMS "Use simulated communications backend" ON)

if(USE_SIM_COMS)
    add_compile_definitions(USE_SIM_COMS)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(qt_app
    main.cpp
)

add_subdirectory(ui)
add_subdirectory(controller)
add_subdirectory(backend)

set_target_properties(qt_app PROPERTIES
    AUTOUIC_SEARCH_PATHS "${CMAKE_SOURCE_DIR}/ui"
)

target_link_libraries(qt_app PRIVATE   
    Qt6::Widgets 
    Qt6::SerialPort 
    Qt6::Network
)
