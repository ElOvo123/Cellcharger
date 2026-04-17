target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/log_widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/log_widget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/log_widget.ui
)
target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)