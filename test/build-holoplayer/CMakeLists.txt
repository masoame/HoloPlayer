project(holoplayer LANGUAGES CXX)
add_executable(${PROJECT_NAME} ${PROJECT_NAME}.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC gui)
set_target_properties(${PROJECT_NAME} PROPERTIES 
   WIN32_EXECUTABLE TRUE
)