cmake_minimum_required(VERSION 3.22)
project(AudioToMidi VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.4
  GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

option(AM_INSTALL "Copy plugin to the system plugin folder after build" OFF)

set(AM_FORMATS VST3 Standalone)
if(APPLE)
  list(APPEND AM_FORMATS AU)
endif()

juce_add_plugin(AudioToMidi
  COMPANY_NAME "AudioToMidi"
  PLUGIN_MANUFACTURER_CODE Atmd
  PLUGIN_CODE Atm1
  FORMATS ${AM_FORMATS}
  PRODUCT_NAME "Audio To MIDI"
  NEEDS_MIDI_OUTPUT TRUE
  NEEDS_MIDI_INPUT FALSE
  IS_SYNTH FALSE
  IS_MIDI_EFFECT FALSE
  AU_MAIN_TYPE kAudioUnitType_MusicEffect
  VST3_CATEGORIES Fx Tools
  COPY_PLUGIN_AFTER_BUILD ${AM_INSTALL})

target_sources(AudioToMidi PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp)

target_compile_definitions(AudioToMidi PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(AudioToMidi
  PRIVATE juce::juce_audio_utils
  PUBLIC juce::juce_recommended_config_flags
         juce::juce_recommended_lto_flags
         juce::juce_recommended_warning_flags)
