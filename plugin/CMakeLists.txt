cmake_minimum_required(VERSION 3.22)
project(OhASynth VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

include(FetchContent)
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.6
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(OhASynth
  COMPANY_NAME "Oh-a-synth"
  BUNDLE_ID com.ohasynth.plugin
  ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icon.png"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  COPY_PLUGIN_AFTER_BUILD TRUE        # installs AU/VST3 into ~/Library/Audio/Plug-Ins
  PLUGIN_MANUFACTURER_CODE Ohas
  PLUGIN_CODE Oha1
  FORMATS AU VST3 Standalone
  PRODUCT_NAME "Oh-a-synth"
)

target_sources(OhASynth PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp
)

# Archivo, the interface face shared with the web app and the product site.
# Compiled in rather than installed, so the plugin looks the same on a machine
# that has never seen the font. Regenerate the .ttf files with tools/make-fonts.sh.
juce_add_binary_data(OhaFonts
  HEADER_NAME OhaFonts.h
  NAMESPACE OhaFonts
  SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/fonts/Archivo-Regular.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/fonts/Archivo-Bold.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/fonts/Archivo-BoldItalic.ttf"
)
set_target_properties(OhaFonts PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_compile_definitions(OhASynth PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(OhASynth
  PRIVATE
    juce::juce_audio_utils
    OhaFonts
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
