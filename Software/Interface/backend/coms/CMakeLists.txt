target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/coms_backend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/coms_backend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/coms_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/icomms_backend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/simulated_coms_backend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/simulated_coms_backend.h
)

target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
