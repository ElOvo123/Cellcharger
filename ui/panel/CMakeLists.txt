target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/panel_container.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panel_container.h
)
target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)