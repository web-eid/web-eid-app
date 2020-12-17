add_library(mock-ui STATIC
    mock-ui.cpp
    mock-ui.hpp
)
target_include_directories(mock-ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mock-ui controller)
