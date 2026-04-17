target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/panel_factory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/panel_factory.cpp
)
target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)