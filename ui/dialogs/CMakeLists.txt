target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/help_dialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/help_dialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/help_dialog.ui
)

target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)