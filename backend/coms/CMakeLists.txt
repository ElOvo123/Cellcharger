target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/coms_backend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/coms_backend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/coms_config.h
)

target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
