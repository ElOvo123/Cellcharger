target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/charger_status_logic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/charger_status_logic.h
    ${CMAKE_CURRENT_SOURCE_DIR}/coms_activity_widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/coms_activity_widget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/charger_history_plot_widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/charger_history_plot_widget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/charger_status_widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/charger_status_widget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/charger_status_widget.ui
)
target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
