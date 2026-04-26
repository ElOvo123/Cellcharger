target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/measurement_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/measurement_engine.h
)

target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/backend/requirements
)
