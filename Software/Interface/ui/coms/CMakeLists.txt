target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/coms.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/coms.h
    ${CMAKE_CURRENT_SOURCE_DIR}/coms.ui
)

target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)