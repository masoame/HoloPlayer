project(ffmpeg)
message("Configuring ffmpeg" ${CMAKE_CURRENT_LIST_DIR})

file(GLOB FFMPEG_PATH ${CMAKE_CURRENT_LIST_DIR}/ffmpeg*shared*)

if(NOT FFMPEG_PATH)
    file(DOWNLOAD 
        https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-n7.1-latest-win64-gpl-shared-7.1.zip
        ffmpeg.zip
    )
    execute_process(COMMAND 
        tar -xzvf ffmpeg.zip -C ${CMAKE_CURRENT_LIST_DIR}
    )
    file(GLOB FFMPEG_PATH ${CMAKE_CURRENT_LIST_DIR}/ffmpeg*shared*)
endif()

file(GLOB LIB_FILES ${FFMPEG_PATH}/lib/*.lib)
file(GLOB DLL_FILES ${FFMPEG_PATH}/bin/*.dll)

add_library(ffmpeg INTERFACE)

target_include_directories(ffmpeg INTERFACE ${FFMPEG_PATH}/include)
target_link_directories(ffmpeg INTERFACE ${FFMPEG_PATH}/bin)
target_link_libraries(ffmpeg INTERFACE ${LIB_FILES})

file(COPY ${DLL_FILES} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})