add_executable(coms_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/coms_tests.cpp
    ${CMAKE_SOURCE_DIR}/ui/coms/coms.cpp
    ${CMAKE_SOURCE_DIR}/ui/coms/coms.h
    ${CMAKE_SOURCE_DIR}/ui/coms/coms.ui
)

target_include_directories(coms_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/backend/coms
    ${CMAKE_SOURCE_DIR}/ui/coms
)

target_link_libraries(coms_tests PRIVATE
    Qt6::Test
    Qt6::Widgets
    Qt6::SerialPort
)

add_executable(core_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/core_tests.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_database.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_encoder.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_decoder.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_formatter.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_decode_formatter.cpp
    ${CMAKE_SOURCE_DIR}/backend/logger/logger_backend.cpp
    ${CMAKE_SOURCE_DIR}/ui/console/console_message_model.cpp
    ${CMAKE_SOURCE_DIR}/ui/console/console_message_model.h
    ${CMAKE_SOURCE_DIR}/ui/console/console_message_filter_proxy_model.cpp
    ${CMAKE_SOURCE_DIR}/ui/console/console_message_filter_proxy_model.h
)

target_include_directories(core_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/backend/logger
    ${CMAKE_SOURCE_DIR}/backend/pcp
    ${CMAKE_SOURCE_DIR}/ui/console
)

target_link_libraries(core_tests PRIVATE
    Qt6::Test
    Qt6::Core
    yaml-cpp
)

add_test(NAME coms_tests COMMAND coms_tests)
set_tests_properties(coms_tests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_test(NAME core_tests COMMAND core_tests)
set_tests_properties(core_tests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_executable(widget_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/widget_tests.cpp
    ${CMAKE_SOURCE_DIR}/backend/logger/logger_backend.cpp
    ${CMAKE_SOURCE_DIR}/ui/dialogs/help_dialog.cpp
    ${CMAKE_SOURCE_DIR}/ui/dialogs/help_dialog.h
    ${CMAKE_SOURCE_DIR}/ui/dialogs/help_dialog.ui
    ${CMAKE_SOURCE_DIR}/ui/log/log_widget.cpp
    ${CMAKE_SOURCE_DIR}/ui/log/log_widget.h
    ${CMAKE_SOURCE_DIR}/ui/log/log_widget.ui
    ${CMAKE_SOURCE_DIR}/ui/charger_status/coms_activity_widget.cpp
    ${CMAKE_SOURCE_DIR}/ui/charger_status/coms_activity_widget.h
    ${CMAKE_SOURCE_DIR}/ui/charger_status/charger_status_widget.cpp
    ${CMAKE_SOURCE_DIR}/ui/charger_status/charger_status_widget.h
    ${CMAKE_SOURCE_DIR}/ui/charger_status/charger_status_widget.ui
    ${CMAKE_SOURCE_DIR}/ui/panel/panel_container.cpp
    ${CMAKE_SOURCE_DIR}/ui/panel/panel_container.h
)

target_include_directories(widget_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/backend/logger
    ${CMAKE_SOURCE_DIR}/ui/dialogs
    ${CMAKE_SOURCE_DIR}/ui/log
    ${CMAKE_SOURCE_DIR}/ui/charger_status
    ${CMAKE_SOURCE_DIR}/ui/panel
)

target_link_libraries(widget_tests PRIVATE
    Qt6::Test
    Qt6::Widgets
)

add_test(NAME widget_tests COMMAND widget_tests)
set_tests_properties(widget_tests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
