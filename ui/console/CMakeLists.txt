target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/console_widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/console_widget.h
    #${CMAKE_CURRENT_SOURCE_DIR}/console_widget.ui
)
target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)