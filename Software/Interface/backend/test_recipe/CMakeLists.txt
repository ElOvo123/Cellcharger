target_sources(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test_recipe.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_recipe.h
)

target_include_directories(qt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/backend/requirements
)
