target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/logger_backend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logger_backend.h
)

target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
