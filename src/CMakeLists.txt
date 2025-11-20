set(SOURCES
    main.cpp
    appcontroller.cpp
    appcontroller.h
    audio/audioproject.cpp
    audio/audioproject.h
    audio/audiobuffer.cpp
    audio/audiobuffer.h
    audio/audiofiledecoder.cpp
    audio/audiofiledecoder.h
    audio/audioplaybackengine.cpp
    audio/audioplaybackengine.h
    audio/recordingengine.cpp
    audio/recordingengine.h
    audio/segmentmodel.cpp
    audio/segmentmodel.h
    audio/wavutils.cpp
    audio/wavutils.h
    audio/volumeanalyzer.cpp
    audio/volumeanalyzer.h
    persistence/projectserializer.cpp
    persistence/projectserializer.h
    utils/pathutils.cpp
    utils/pathutils.h
    utils/logger.h
)

set(RESOURCES
    ../resources/qml.qrc
)

add_library(minimp3 INTERFACE)
target_include_directories(minimp3 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty)

add_executable(voice_upside_down
    ${SOURCES}
    ${RESOURCES}
)

target_include_directories(voice_upside_down PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Set UTF-8 encoding for source files (important for MSVC on Windows)
if (MSVC)
    target_compile_options(voice_upside_down PRIVATE /utf-8)
endif()

target_link_libraries(voice_upside_down PRIVATE
    Qt5::Widgets
    Qt5::Quick
    Qt5::Qml
    Qt5::Multimedia
    minimp3
)

if (WIN32)
    set_target_properties(voice_upside_down PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

