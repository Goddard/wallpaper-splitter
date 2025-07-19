cmake_minimum_required(VERSION 3.16)
project(wallpaper-splitter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
find_package(KF6CoreAddons REQUIRED)
find_package(KF6WidgetsAddons REQUIRED)
find_package(KF6I18n REQUIRED)

# Qt provides cross-platform screen detection (works on X11 and Wayland)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Core library
set(CORE_SOURCES
    src/core/image_splitter.cpp
    src/core/monitor_detector.cpp
    src/core/wallpaper_applier.cpp
)

# Process Qt MOC for core library
qt_wrap_cpp(CORE_MOC include/core/monitor_detector.h include/core/wallpaper_applier.h)

add_library(wallpaper-core STATIC
    ${CORE_SOURCES}
    ${CORE_MOC}
)

target_link_libraries(wallpaper-core
    Qt6::Core
    Qt6::Gui
)

# KDE Plasma interface
set(KDE_SOURCES
    src/kde/main.cpp
    src/kde/mainwindow.cpp
    src/kde/monitorwidget.cpp
    src/kde/imagepreview.cpp
)

# Process Qt MOC for KDE interface
qt_wrap_cpp(KDE_MOC 
    src/kde/mainwindow.h
    src/kde/monitorwidget.h
    src/kde/imagepreview.h
)

add_executable(wallpaper-splitter-kde
    ${KDE_SOURCES}
    ${KDE_MOC}
)

target_link_libraries(wallpaper-splitter-kde
    wallpaper-core
    Qt6::Core
    Qt6::Widgets
    KF6::CoreAddons
    KF6::WidgetsAddons
    KF6::I18n
)

# Command line interface
add_executable(wallpaper-splitter-cli
    src/cli/main.cpp
)

target_link_libraries(wallpaper-splitter-cli
    wallpaper-core
    Qt6::Core
)

# Install targets
install(TARGETS wallpaper-splitter-kde wallpaper-splitter-cli
    RUNTIME DESTINATION bin
) 