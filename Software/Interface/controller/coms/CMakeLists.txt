target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/coms_controller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/coms_controller.cpp
)

target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)