cmake_minimum_required(VERSION 3.16)
project(TdimensionMC VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 按顺序查找 Qt6 或 Qt5，找到后设置 QT_VERSION_MAJOR
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Multimedia)

# 所有源文件（根据你的实际文件调整）
set(SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    gamewidget.cpp
    gamewidget.h
    dirtblock.cpp
    dirtblock.h
    oreblock.cpp
    oreblock.h
    treeblock.cpp
    treeblock.h
)

add_executable(TdimensionMC ${SOURCES})

target_link_libraries(TdimensionMC PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
)


if(APPLE)
    set_target_properties(TdimensionMC PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    )
endif()
