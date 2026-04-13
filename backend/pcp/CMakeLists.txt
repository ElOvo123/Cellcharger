target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/pcp_encoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pcp_formatter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pcp_formatter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pcp_encoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pcp_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pcp_decoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pcp_decode_formatter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pcp_decode_formatter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pcp_database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pcp_database.h
)

target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
