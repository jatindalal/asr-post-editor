cmake_minimum_required(VERSION 3.21)

project(asr-post-editor)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOUIC_SEARCH_PATHS)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)

find_package (
    Qt5
    HINTS "$ENV{QTDIR}"
    REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Multimedia
    MultimediaWidgets
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB PROJECT_FORMS "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")
file(GLOB PROJECT_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB PROJECT_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB PROJECT_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")

set(GUI_TYPE)
if (WIN32)
    set(GUI_TYPE WIN32)
else ()
    set(GUI_TYPE)
endif ()

add_executable(
        ${PROJECT_NAME}
        ${GUI_TYPE} main.cpp
        ${PROJECT_FORMS}
        ${PROJECT_SOURCE}
        ${PROJECT_HEADER}
        ${PROJECT_RESOURCES}
)


target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Multimedia
        Qt5::MultimediaWidgets
)
