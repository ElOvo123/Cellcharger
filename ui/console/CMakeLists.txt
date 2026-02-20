target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/console.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/console.h
)
target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)