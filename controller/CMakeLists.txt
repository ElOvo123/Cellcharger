target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow_controller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow_controller.h
)

target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
