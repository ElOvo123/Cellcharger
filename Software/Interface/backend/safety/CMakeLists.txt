target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/safety_monitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/safety_monitor.h
)

target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
