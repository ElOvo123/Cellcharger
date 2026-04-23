add_library(profile_setup STATIC
    profile_plot_logic.cpp
    profile_plot_logic.h
    profile_setup_dialogs.h
    profile_setup_logic.cpp
    profile_setup_logic.h
    profile_yaml_logic.cpp
    profile_yaml_logic.h
    profile_setup_widget.cpp
    profile_setup_widget.h
    profile_setup_widget.ui
    profile_plot_widget.cpp
    profile_plot_widget.h
)

target_link_libraries(profile_setup
    Qt6::Core
    Qt6::Widgets
    yaml-cpp
)

target_include_directories(profile_setup PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/backend/logger
)
