target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/interface_backend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/interface_backend.h
)

target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
