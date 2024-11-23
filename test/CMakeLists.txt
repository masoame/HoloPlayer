project(test)

file(GLOB ALL_FILES ${CMAKE_CURRENT_LIST_DIR}/*)
foreach(var ${ALL_FILES})
    if(IS_DIRECTORY ${var})
        add_subdirectory(${var})
    endif()
endforeach()