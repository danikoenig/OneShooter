cmake_minimum_required(VERSION 3.22)

project(OneShooter VERSION 1.4.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: lokaler Pfad (-DJUCE_PATH=...) oder automatischer Download
if(DEFINED JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(OneShooter
    COMPANY_NAME "Dani Koenig"
    BUNDLE_ID "ch.danikoenig.oneshooter"
    PLUGIN_MANUFACTURER_CODE Dkng
    PLUGIN_CODE Kck1
    FORMATS VST3 AU Standalone
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    PRODUCT_NAME "One Shooter"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Drum Sampler)

target_sources(OneShooter PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/WaveformView.cpp
    Source/SampleLibrary.cpp
    Source/LibraryPanel.cpp)

target_compile_definitions(OneShooter PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(OneShooter
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)


if(SHOOTER_TEST_SRC)
    add_executable(ShooterTest ${SHOOTER_TEST_SRC})
    target_link_libraries(ShooterTest PRIVATE OneShooter)
    target_include_directories(ShooterTest PRIVATE $<TARGET_PROPERTY:OneShooter,INCLUDE_DIRECTORIES>)
    target_compile_definitions(ShooterTest PRIVATE $<TARGET_PROPERTY:OneShooter,COMPILE_DEFINITIONS>)
endif()
