target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/pcp_encoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pcp_formatter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pcp_formatter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pcp_encoder.h
)

target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
