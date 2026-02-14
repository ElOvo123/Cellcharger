target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow_view.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow_view.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc
)
target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)