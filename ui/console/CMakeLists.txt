target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/console_widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/console_widget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/console_message_model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/console_message_model.h
    ${CMAKE_CURRENT_SOURCE_DIR}/console_message_filter_proxy_model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/console_message_filter_proxy_model.h
    ${CMAKE_CURRENT_SOURCE_DIR}/console_widget.ui
)
target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)