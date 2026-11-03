cmake_minimum_required(VERSION 3.22)
project(BASSBOXX VERSION 1.0.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)
option(BASSBOXX_ENGINE_ONLY "Build portable DSP tests without JUCE" OFF)
add_executable(engine_tests tests/EngineTests.cpp)
target_include_directories(engine_tests PRIVATE Source)
add_test(NAME engine COMMAND engine_tests)
set_tests_properties(engine PROPERTIES TIMEOUT 120)
add_executable(render_demo tests/RenderDemo.cpp)
target_include_directories(render_demo PRIVATE Source)
if(BASSBOXX_ENGINE_ONLY)
 return()
endif()
set(JUCE_SOURCE_DIR "" CACHE PATH "Existing JUCE 8.0.6 checkout; otherwise fetched automatically")
if(JUCE_SOURCE_DIR)
 add_subdirectory("${JUCE_SOURCE_DIR}" JUCE)
else()
 include(FetchContent)
 FetchContent_Declare(JUCE
  URL https://codeload.github.com/juce-framework/JUCE/tar.gz/51a8a6d7aeae7326956d747737ccf1575e61e209
  URL_HASH SHA256=20b740351cc030086951d87614bc08b42d072f1e4db68db1ef1d988e66dbbdbe)
 FetchContent_MakeAvailable(JUCE)
endif()
set(BASSBOXX_FORMATS)
if(APPLE)
 set(BASSBOXX_FORMATS VST3 Standalone)
else()
 message(STATUS "Non-macOS: shared-code tests only; no instrument binary is produced")
endif()
juce_add_plugin(BASSBOXX
 COMPANY_NAME "ProHoeZak" BUNDLE_ID "com.prohoezak.bassboxx"
 PLUGIN_MANUFACTURER_CODE PhZk PLUGIN_CODE BsBx
 FORMATS ${BASSBOXX_FORMATS} PRODUCT_NAME "BASSBOXX"
 IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE
 IS_MIDI_EFFECT FALSE EDITOR_WANTS_KEYBOARD_FOCUS FALSE
 COPY_PLUGIN_AFTER_BUILD FALSE VST3_CATEGORIES Instrument Synth)
if(TARGET BASSBOXX_Standalone)
 target_sources(BASSBOXX_Standalone PRIVATE Source/Standalone.cpp)
 target_compile_definitions(BASSBOXX_Standalone PRIVATE JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)
 add_test(NAME standalone COMMAND "${CMAKE_COMMAND}"
  "-DAPP=$<TARGET_BUNDLE_DIR:BASSBOXX_Standalone>"
  "-DREPORT=${CMAKE_CURRENT_BINARY_DIR}/standalone-report.json"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/StandaloneSmoke.cmake")
 set_tests_properties(standalone PROPERTIES TIMEOUT 40)
endif()
juce_generate_juce_header(BASSBOXX)
target_sources(BASSBOXX PRIVATE Source/Plugin.cpp Source/Plugin.h Source/Editor.h Source/Engine.h)
target_compile_definitions(BASSBOXX PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(BASSBOXX PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags)
add_executable(plugin_tests tests/PluginTests.cpp)
target_link_libraries(plugin_tests PRIVATE BASSBOXX juce::juce_audio_utils)
target_include_directories(plugin_tests PRIVATE Source "${CMAKE_CURRENT_BINARY_DIR}/BASSBOXX_artefacts/JuceLibraryCode")
add_test(NAME plugin COMMAND plugin_tests)
set_tests_properties(plugin PROPERTIES TIMEOUT 120)

# Exercise the compiled VST3 through JUCE's actual host wrapper, rather than
# testing only the in-process AudioProcessor C++ implementation.
if(APPLE)
 add_executable(vst3_host_smoke tests/VST3HostSmoke.cpp)
 target_compile_definitions(vst3_host_smoke PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_PLUGINHOST_AU=0 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
 target_link_libraries(vst3_host_smoke PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags)
 add_dependencies(vst3_host_smoke BASSBOXX_VST3)
 add_test(NAME vst3_host_load COMMAND vst3_host_smoke "$<TARGET_BUNDLE_DIR:BASSBOXX_VST3>")
 add_test(NAME vst3_scan COMMAND vst3_host_smoke "$<TARGET_BUNDLE_DIR:BASSBOXX_VST3>" --scan-only)
 set_tests_properties(vst3_host_load vst3_scan PROPERTIES TIMEOUT 30)
endif()
