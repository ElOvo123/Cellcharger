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
target_project_warnings(coms_tests)

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
target_project_warnings(core_tests)

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

add_executable(backend_coms_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/backend_coms_tests.cpp
    ${CMAKE_SOURCE_DIR}/backend/coms/coms_backend.cpp
    ${CMAKE_SOURCE_DIR}/backend/coms/coms_backend.h
    ${CMAKE_SOURCE_DIR}/backend/coms/icomms_backend.h
    ${CMAKE_SOURCE_DIR}/backend/coms/coms_types.h
    ${CMAKE_SOURCE_DIR}/backend/logger/logger_backend.cpp
    ${CMAKE_SOURCE_DIR}/backend/logger/logger_backend.h
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_database.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_database.h
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_formatter.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_formatter.h
)

target_include_directories(backend_coms_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/backend/coms
    ${CMAKE_SOURCE_DIR}/backend/logger
    ${CMAKE_SOURCE_DIR}/backend/pcp
)

target_link_libraries(backend_coms_tests PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    Qt6::SerialPort
    yaml-cpp
)
target_project_warnings(backend_coms_tests)

add_test(NAME backend_coms_tests COMMAND backend_coms_tests)
set_tests_properties(backend_coms_tests PROPERTIES
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
    ${CMAKE_SOURCE_DIR}/ui/console/console_message_filter_proxy_model.cpp
    ${CMAKE_SOURCE_DIR}/ui/console/console_message_filter_proxy_model.h
    ${CMAKE_SOURCE_DIR}/ui/console/console_message_model.cpp
    ${CMAKE_SOURCE_DIR}/ui/console/console_message_model.h
    ${CMAKE_SOURCE_DIR}/ui/console/console_widget.cpp
    ${CMAKE_SOURCE_DIR}/ui/console/console_widget.h
    ${CMAKE_SOURCE_DIR}/ui/console/console_widget.ui
    ${CMAKE_SOURCE_DIR}/ui/charger_status/coms_activity_widget.cpp
    ${CMAKE_SOURCE_DIR}/ui/charger_status/coms_activity_widget.h
    ${CMAKE_SOURCE_DIR}/ui/charger_status/charger_history_plot_widget.cpp
    ${CMAKE_SOURCE_DIR}/ui/charger_status/charger_history_plot_widget.h
    ${CMAKE_SOURCE_DIR}/ui/charger_status/charger_status_logic.cpp
    ${CMAKE_SOURCE_DIR}/ui/charger_status/charger_status_logic.h
    ${CMAKE_SOURCE_DIR}/ui/charger_status/charger_status_widget.cpp
    ${CMAKE_SOURCE_DIR}/ui/charger_status/charger_status_widget.h
    ${CMAKE_SOURCE_DIR}/ui/charger_status/charger_status_widget.ui
    ${CMAKE_SOURCE_DIR}/ui/panel/panel_container.cpp
    ${CMAKE_SOURCE_DIR}/ui/panel/panel_container.h
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_database.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_database.h
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_decoder.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_decoder.h
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_decode_formatter.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_decode_formatter.h
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_encoder.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_encoder.h
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_formatter.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_formatter.h
)

target_include_directories(widget_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/backend/logger
    ${CMAKE_SOURCE_DIR}/backend/pcp
    ${CMAKE_SOURCE_DIR}/ui/console
    ${CMAKE_SOURCE_DIR}/ui/dialogs
    ${CMAKE_SOURCE_DIR}/ui/log
    ${CMAKE_SOURCE_DIR}/ui/charger_status
    ${CMAKE_SOURCE_DIR}/ui/panel
)

target_link_libraries(widget_tests PRIVATE
    Qt6::Test
    Qt6::Widgets
    yaml-cpp
)
target_project_warnings(widget_tests)

add_test(NAME widget_tests COMMAND widget_tests)
set_tests_properties(widget_tests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_executable(profile_setup_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/profile_setup_tests.cpp
    ${CMAKE_SOURCE_DIR}/ui/profile_setup/profile_plot_logic.cpp
    ${CMAKE_SOURCE_DIR}/ui/profile_setup/profile_plot_logic.h
    ${CMAKE_SOURCE_DIR}/ui/profile_setup/profile_plot_widget.cpp
    ${CMAKE_SOURCE_DIR}/ui/profile_setup/profile_setup_dialogs.h
    ${CMAKE_SOURCE_DIR}/ui/profile_setup/profile_setup_logic.cpp
    ${CMAKE_SOURCE_DIR}/ui/profile_setup/profile_setup_logic.h
    ${CMAKE_SOURCE_DIR}/ui/profile_setup/profile_yaml_logic.cpp
    ${CMAKE_SOURCE_DIR}/ui/profile_setup/profile_yaml_logic.h
    ${CMAKE_SOURCE_DIR}/ui/profile_setup/profile_setup_widget.cpp
    ${CMAKE_SOURCE_DIR}/ui/profile_setup/profile_setup_widget.h
    ${CMAKE_SOURCE_DIR}/ui/profile_setup/profile_setup_widget.ui
    ${CMAKE_SOURCE_DIR}/backend/logger/logger_backend.cpp
)

target_include_directories(profile_setup_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/ui/profile_setup
    ${CMAKE_SOURCE_DIR}/backend/logger
)

target_link_libraries(profile_setup_tests PRIVATE
    Qt6::Test
    Qt6::Widgets
    yaml-cpp
)
target_project_warnings(profile_setup_tests)

add_test(NAME profile_setup_tests COMMAND profile_setup_tests)
set_tests_properties(profile_setup_tests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_executable(integration_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/integration_tests.cpp
    ${CMAKE_SOURCE_DIR}/ui/coms/coms.cpp
    ${CMAKE_SOURCE_DIR}/ui/coms/coms.h
    ${CMAKE_SOURCE_DIR}/ui/coms/coms.ui
    ${CMAKE_SOURCE_DIR}/controller/coms/coms_controller.cpp
    ${CMAKE_SOURCE_DIR}/controller/coms/coms_controller.h
    ${CMAKE_SOURCE_DIR}/backend/coms/coms_backend.cpp
    ${CMAKE_SOURCE_DIR}/backend/coms/coms_backend.h
    ${CMAKE_SOURCE_DIR}/backend/coms/icomms_backend.h
    ${CMAKE_SOURCE_DIR}/backend/coms/simulated_coms_backend.cpp
    ${CMAKE_SOURCE_DIR}/backend/coms/simulated_coms_backend.h
    ${CMAKE_SOURCE_DIR}/backend/coms/coms_types.h
    ${CMAKE_SOURCE_DIR}/backend/logger/logger_backend.cpp
    ${CMAKE_SOURCE_DIR}/backend/logger/logger_backend.h
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_database.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_database.h
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_decoder.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_decoder.h
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_decode_formatter.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_decode_formatter.h
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_encoder.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_encoder.h
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_formatter.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_formatter.h
)

target_include_directories(integration_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/backend/coms
    ${CMAKE_SOURCE_DIR}/backend/logger
    ${CMAKE_SOURCE_DIR}/backend/pcp
    ${CMAKE_SOURCE_DIR}/controller/coms
    ${CMAKE_SOURCE_DIR}/ui/coms
    ${CMAKE_SOURCE_DIR}/ui/profile_setup
)

target_link_libraries(integration_tests PRIVATE
    Qt6::Test
    Qt6::Widgets
    Qt6::Network
    Qt6::SerialPort
    yaml-cpp
    profile_setup
)
target_project_warnings(integration_tests)

add_test(NAME integration_tests COMMAND integration_tests)
set_tests_properties(integration_tests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_executable(smoke_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/smoke_tests.cpp
    ${CMAKE_SOURCE_DIR}/backend/coms/coms_backend.cpp
    ${CMAKE_SOURCE_DIR}/backend/coms/coms_backend.h
    ${CMAKE_SOURCE_DIR}/backend/coms/icomms_backend.h
    ${CMAKE_SOURCE_DIR}/backend/coms/simulated_coms_backend.cpp
    ${CMAKE_SOURCE_DIR}/backend/coms/simulated_coms_backend.h
    ${CMAKE_SOURCE_DIR}/backend/coms/coms_types.h
    ${CMAKE_SOURCE_DIR}/backend/logger/logger_backend.cpp
    ${CMAKE_SOURCE_DIR}/backend/logger/logger_backend.h
    ${CMAKE_SOURCE_DIR}/backend/mainwindow/interface_backend.cpp
    ${CMAKE_SOURCE_DIR}/backend/mainwindow/interface_backend.h
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_database.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_database.h
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_decoder.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_decoder.h
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_decode_formatter.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_decode_formatter.h
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_encoder.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_encoder.h
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_formatter.cpp
    ${CMAKE_SOURCE_DIR}/backend/pcp/pcp_formatter.h
    ${CMAKE_SOURCE_DIR}/controller/coms/coms_controller.cpp
    ${CMAKE_SOURCE_DIR}/controller/coms/coms_controller.h
    ${CMAKE_SOURCE_DIR}/controller/mainwindow/mainwindow_controller.cpp
    ${CMAKE_SOURCE_DIR}/controller/mainwindow/mainwindow_controller.h
    ${CMAKE_SOURCE_DIR}/ui/charger_status/charger_history_plot_widget.cpp
    ${CMAKE_SOURCE_DIR}/ui/charger_status/charger_history_plot_widget.h
    ${CMAKE_SOURCE_DIR}/ui/charger_status/charger_status_logic.cpp
    ${CMAKE_SOURCE_DIR}/ui/charger_status/charger_status_logic.h
    ${CMAKE_SOURCE_DIR}/ui/charger_status/charger_status_widget.cpp
    ${CMAKE_SOURCE_DIR}/ui/charger_status/charger_status_widget.h
    ${CMAKE_SOURCE_DIR}/ui/charger_status/charger_status_widget.ui
    ${CMAKE_SOURCE_DIR}/ui/charger_status/coms_activity_widget.cpp
    ${CMAKE_SOURCE_DIR}/ui/charger_status/coms_activity_widget.h
    ${CMAKE_SOURCE_DIR}/ui/coms/coms.cpp
    ${CMAKE_SOURCE_DIR}/ui/coms/coms.h
    ${CMAKE_SOURCE_DIR}/ui/coms/coms.ui
    ${CMAKE_SOURCE_DIR}/ui/console/console_message_filter_proxy_model.cpp
    ${CMAKE_SOURCE_DIR}/ui/console/console_message_filter_proxy_model.h
    ${CMAKE_SOURCE_DIR}/ui/console/console_message_model.cpp
    ${CMAKE_SOURCE_DIR}/ui/console/console_message_model.h
    ${CMAKE_SOURCE_DIR}/ui/console/console_widget.cpp
    ${CMAKE_SOURCE_DIR}/ui/console/console_widget.h
    ${CMAKE_SOURCE_DIR}/ui/console/console_widget.ui
    ${CMAKE_SOURCE_DIR}/ui/dialogs/help_dialog.cpp
    ${CMAKE_SOURCE_DIR}/ui/dialogs/help_dialog.h
    ${CMAKE_SOURCE_DIR}/ui/dialogs/help_dialog.ui
    ${CMAKE_SOURCE_DIR}/ui/log/log_widget.cpp
    ${CMAKE_SOURCE_DIR}/ui/log/log_widget.h
    ${CMAKE_SOURCE_DIR}/ui/log/log_widget.ui
    ${CMAKE_SOURCE_DIR}/ui/mainwindow/mainwindow_view.cpp
    ${CMAKE_SOURCE_DIR}/ui/mainwindow/mainwindow_view.h
    ${CMAKE_SOURCE_DIR}/ui/mainwindow/mainwindow.ui
    ${CMAKE_SOURCE_DIR}/ui/mainwindow/resources.qrc
    ${CMAKE_SOURCE_DIR}/ui/panel/panel_container.cpp
    ${CMAKE_SOURCE_DIR}/ui/panel/panel_container.h
    ${CMAKE_SOURCE_DIR}/ui/panels/panel_factory.cpp
    ${CMAKE_SOURCE_DIR}/ui/panels/panel_factory.h
)

target_include_directories(smoke_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/backend/coms
    ${CMAKE_SOURCE_DIR}/backend/logger
    ${CMAKE_SOURCE_DIR}/backend/mainwindow
    ${CMAKE_SOURCE_DIR}/backend/pcp
    ${CMAKE_SOURCE_DIR}/controller/coms
    ${CMAKE_SOURCE_DIR}/controller/mainwindow
    ${CMAKE_SOURCE_DIR}/ui/charger_status
    ${CMAKE_SOURCE_DIR}/ui/coms
    ${CMAKE_SOURCE_DIR}/ui/console
    ${CMAKE_SOURCE_DIR}/ui/dialogs
    ${CMAKE_SOURCE_DIR}/ui/log
    ${CMAKE_SOURCE_DIR}/ui/mainwindow
    ${CMAKE_SOURCE_DIR}/ui/panel
    ${CMAKE_SOURCE_DIR}/ui/panels
    ${CMAKE_SOURCE_DIR}/ui/profile_setup
)

target_link_libraries(smoke_tests PRIVATE
    Qt6::Test
    Qt6::Widgets
    Qt6::Network
    Qt6::SerialPort
    yaml-cpp
    profile_setup
)
target_project_warnings(smoke_tests)

add_test(NAME smoke_tests COMMAND smoke_tests)
set_tests_properties(smoke_tests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

set_tests_properties(
    coms_tests
    core_tests
    backend_coms_tests
    widget_tests
    profile_setup_tests
    PROPERTIES
        LABELS "unit"
        TIMEOUT 30
)

set_tests_properties(integration_tests PROPERTIES
    LABELS "integration"
    TIMEOUT 30
)

set_tests_properties(smoke_tests PROPERTIES
    LABELS "smoke"
    TIMEOUT 30
)
