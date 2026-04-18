add_library(profile_setup STATIC
    profile_setup_widget.cpp
    profile_setup_widget.h
    profile_setup_widget.ui
    profile_plot_widget.cpp
    profile_plot_widget.h
)

target_link_libraries(profile_setup
    Qt6::Core
    Qt6::Widgets
)

target_include_directories(profile_setup PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})