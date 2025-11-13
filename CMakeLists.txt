cmake_minimum_required(VERSION 3.16)

project(voice_upside_down LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# Android configuration
if (ANDROID)
    set(ANDROID_PACKAGE_NAME "com.voiceupsidedown.master")
    set(ANDROID_APP_ICON "@mipmap/ic_launcher")
    set(ANDROID_MIN_SDK_VERSION 21)
    set(ANDROID_TARGET_SDK_VERSION 33)
endif()

set(QT_MIN_VERSION "5.15.0")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 ${QT_MIN_VERSION} COMPONENTS
    Widgets
    Quick
    Qml
    Multimedia
    REQUIRED
)

# QtGraphicalEffects is optional but recommended for visual effects
find_package(Qt5 ${QT_MIN_VERSION} COMPONENTS QuickWidgets QUIET)
if(Qt5QuickWidgets_FOUND)
    # QtGraphicalEffects is available through QuickWidgets
endif()

add_subdirectory(src)

