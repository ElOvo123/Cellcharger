target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/calibration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/calibration.h
)

target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
