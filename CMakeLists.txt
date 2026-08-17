cmake_minimum_required(VERSION 3.22)
project(SCRAPER VERSION 0.2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(JUCE_DIR "" CACHE PATH "Optional path to a local JUCE checkout")
if(EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory("${JUCE_DIR}" JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.11
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(SCRAPER_MAX_SECONDS 180 CACHE STRING "Maximum decoded sample length")

juce_add_plugin(Scraper
    COMPANY_NAME "Arsider"
    BUNDLE_ID "org.arsider.scraper"
    PLUGIN_MANUFACTURER_CODE Arsd
    PLUGIN_CODE Scr1
    FORMATS VST3 Standalone
    PRODUCT_NAME "SCRAPER"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Scraper PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/ScraperCore.h
    Source/TransientDetector.h)

target_compile_definitions(Scraper PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    SCRAPER_MAX_SECONDS=${SCRAPER_MAX_SECONDS})

target_link_libraries(Scraper PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)

juce_generate_juce_header(Scraper)
