cmake_minimum_required(VERSION 3.8)

project(holoPlayer VERSION 0.1 LANGUAGES CXX)

##-----------------------------------------------------------------------------------------------------------------------------
##--------------------------使用VS打开----CMAKE_PREFIX_PATH请更换成自己本地Qt msvc所在路径-------------------------------------
##---------------------------------------如果使用Qt打开CMake请注释掉这一段-----------------------------------------------------
##-----------------------------------------------------------------------------------------------------------------------------
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()
set(CMAKE_CXX_FLAGS "/EHsc")


set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG /OPT:REF /OPT:ICF")
set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /DEBUG /OPT:REF /OPT:ICF")


set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /O2")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")

set(CMAKE_PREFIX_PATH "D:/Qt/6.7.0/msvc2019_64")
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/out/build)
file(COPY ${CMAKE_PREFIX_PATH}/plugins DESTINATION ${PROJECT_SOURCE_DIR}/out/build)
file(GLOB DLL_FILE ${CMAKE_PREFIX_PATH}/bin/*.dll)
file(COPY ${DLL_FILE} DESTINATION ${PROJECT_SOURCE_DIR}/out/build)

##----------------------------------------------------------------------------------------------------------------------------

set(QT_MOC_EXECUTABLE ${CMAKE_PREFIX_PATH}/bin/moc.exe) 
set(QT_RCC_EXECUTABLE ${CMAKE_PREFIX_PATH}/bin/RCC.exe) 
set(QT_UIC_EXECUTABLE ${CMAKE_PREFIX_PATH}/bin/Uic.exe)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIB_DIR ${PROJECT_SOURCE_DIR}/lib)


include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/inc)
include_directories(${PROJECT_SOURCE_DIR}/ui)

file(GLOB LIB_FILE ${LIB_DIR}/*.lib ${LIB_DIR}/*.a)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS OpenGLWidgets)



set(PROJECT_SOURCES
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(holoPlayer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        src/main.cpp src/ParseJson.cpp src/SpiderVideo.cpp src/BaseFFmpeg.cpp src/BaseSDL.cpp
        inc/BaseFFmpeg.h inc/BaseSDL.h inc/common.hpp inc/Circular_Queue.hpp inc/SpiderVideo.h inc/ParseJson.h inc/EncodeString.hpp
        Img.qrc
        ui/nettofiledialog.h ui/nettofiledialog.cpp ui/nettofiledialog.ui ui/holomainwindow.cpp ui/holomainwindow.h ui/holomainwindow.ui

    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET holoPlayer APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(holoPlayer SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(holoPlayer
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(holoPlayer PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    ${LIB_FILE}
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.holoPlayer)
endif()
set_target_properties(holoPlayer PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUES
    WIN32_EXECUTABLE FALSE
)

include(GNUInstallDirs)
install(TARGETS holoPlayer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(holoPlayer)
endif()
