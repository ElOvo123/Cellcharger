target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/data_log_writer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data_log_writer.h
)

target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/backend/measurements
)
