project(gui LANGUAGES CXX)

set(CMAKE_PREFIX_PATH "D:/Qt/6.7.0/msvc2019_64")

if(CMAKE_PREFIX_PATH STREQUAL "")
    message(STATUS "CMAKE_PREFIX_PATH is empty")
else()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")

        file(GLOB DLL_FILES ${CMAKE_PREFIX_PATH}/bin/*d.dll)
        file(COPY ${DLL_FILES} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

        file(GLOB PLUGIN_PATHS ${CMAKE_PREFIX_PATH}/plugins/*)
        foreach(PLUGIN_PATH ${PLUGIN_PATHS})
            get_filename_component(PLUGIN_NAME ${PLUGIN_PATH} NAME)
            file(GLOB VAR_FILES ${PLUGIN_PATH}/*d.dll)
            file(COPY ${VAR_FILES} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/plugins/${PLUGIN_NAME})
        endforeach()

    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")

        file(GLOB DLL_FILES ${CMAKE_PREFIX_PATH}/bin/*.dll)
        foreach(DLL_FILE ${DLL_FILES})
            get_filename_component(FILENAME ${DLL_FILE} NAME)
            if(NOT FILENAME MATCHES ".*d.dll")
                list(APPEND TARGET_DLLS ${DLL_FILE})
            endif()
        endforeach()
        file(COPY ${TARGET_DLLS} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

        file(GLOB PLUGIN_PATHS ${CMAKE_PREFIX_PATH}/plugins/*)
        foreach(PLUGIN_PATH ${PLUGIN_PATHS})
            set(TARGET_DLLS)
            get_filename_component(PLUGIN_NAME ${PLUGIN_PATH} NAME)

            file(GLOB VAR_FILES ${PLUGIN_PATH}/*.dll)
            foreach(VAR_FILE ${VAR_FILES})
                if(NOT VAR_FILE MATCHES ".*d.dll")
                    list(APPEND TARGET_DLLS ${VAR_FILE})
                endif()
            endforeach()

            file(COPY ${TARGET_DLLS} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/plugins/${PLUGIN_NAME})
        endforeach()

    endif()

    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS OpenGLWidgets)

    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)


    file(GLOB ADD_UI_FILES ${PROJECT_SOURCE_DIR}/*.ui ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.h)


    qt_add_library(${PROJECT_NAME} SHARED
       MANUAL_FINALIZATION
        ${ADD_UI_FILES}
        Img.qrc
    )


    target_link_libraries(${PROJECT_NAME} PUBLIC
        media
        spider
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    )

    target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

endif()