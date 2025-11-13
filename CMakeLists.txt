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
    set(ANDROID_MIN_SDK_VERSION 23)  # Minimum required by Qt 6.7.2 Android Kit
    set(ANDROID_TARGET_SDK_VERSION 33)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Try to find Qt6 first, then fall back to Qt5
# For Android builds, require Qt6
if(ANDROID)
    # Debug: Print paths
    message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
    message(STATUS "CMAKE_FIND_ROOT_PATH: ${CMAKE_FIND_ROOT_PATH}")
    
    # For Android, CMAKE_FIND_ROOT_PATH is typically set by Qt Creator
    # Try to find Qt6 using CMAKE_FIND_ROOT_PATH first
    if(CMAKE_FIND_ROOT_PATH)
        list(GET CMAKE_FIND_ROOT_PATH 0 QT_PREFIX)
        set(Qt6_DIR "${QT_PREFIX}/lib/cmake/Qt6" CACHE PATH "Qt6 directory")
        message(STATUS "Setting Qt6_DIR from CMAKE_FIND_ROOT_PATH to: ${Qt6_DIR}")
    elseif(CMAKE_PREFIX_PATH)
        list(GET CMAKE_PREFIX_PATH 0 QT_PREFIX)
        set(Qt6_DIR "${QT_PREFIX}/lib/cmake/Qt6" CACHE PATH "Qt6 directory")
        message(STATUS "Setting Qt6_DIR from CMAKE_PREFIX_PATH to: ${Qt6_DIR}")
    endif()
    
    # Verify Qt6_DIR exists
    if(Qt6_DIR AND EXISTS "${Qt6_DIR}/Qt6Config.cmake")
        message(STATUS "Qt6_DIR verified: ${Qt6_DIR}/Qt6Config.cmake exists")
    else()
        message(WARNING "Qt6_DIR may be incorrect: ${Qt6_DIR}")
    endif()
    
    # Try to find Qt6
    # Note: Multimedia might not be available for Android in some Qt installations
    find_package(Qt6 6.7.0 COMPONENTS
        Core
        Gui
        Quick
        Qml
        REQUIRED
    )
    
    # Try to find Multimedia separately
    find_package(Qt6Multimedia 6.7.0 QUIET)
    if(NOT Qt6Multimedia_FOUND)
        message(WARNING "Qt6Multimedia not found for Android. Audio features may not work.")
        message(WARNING "Please install Qt6Multimedia for Android via Qt Maintenance Tool:")
        message(WARNING "  Qt 6.7.2 → Android → Android arm64-v8a → Qt Multimedia")
    else()
        message(STATUS "Qt6Multimedia found for Android")
    endif()
    
    # If we get here, Qt6 was found
    message(STATUS "Using Qt 6 for Android")
    set(QT_VERSION_MAJOR 6)
    set(QT_MIN_VERSION "6.7.0")
else()
    # For desktop, try Qt6 first, then Qt5
    find_package(Qt6 6.7.0 COMPONENTS
        Core
        Gui
        Quick
        Qml
        Multimedia
        QUIET
    )
    
    if(Qt6_FOUND)
        message(STATUS "Using Qt 6")
        set(QT_VERSION_MAJOR 6)
        set(QT_MIN_VERSION "6.7.0")
    else()
        message(STATUS "Qt 6 not found, trying Qt 5")
        set(QT_MIN_VERSION "5.15.0")
        find_package(Qt5 ${QT_MIN_VERSION} COMPONENTS
            Widgets
            Quick
            Qml
            Multimedia
            REQUIRED
        )
        set(QT_VERSION_MAJOR 5)
        
        # QtGraphicalEffects is optional but recommended for visual effects
        find_package(Qt5 ${QT_MIN_VERSION} COMPONENTS QuickWidgets QUIET)
        if(Qt5QuickWidgets_FOUND)
            # QtGraphicalEffects is available through QuickWidgets
        endif()
    endif()
endif()

add_subdirectory(src)

