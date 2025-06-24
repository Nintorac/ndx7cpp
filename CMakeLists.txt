cmake_minimum_required(VERSION 3.15)

project(ND7MidiDevice VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find GTK3 and webkit2gtk
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(WEBKIT2GTK REQUIRED webkit2gtk-4.1)

# Add JUCE
add_subdirectory(JUCE)

# Add libtorch
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

# Create the plugin target
juce_add_plugin(ND7MidiDevice
    COMPANY_NAME "ND7"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_PLUGIN TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Nd7c
    PLUGIN_CODE Nd7m
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "ND7 MIDI Device")

# Add source files
target_sources(ND7MidiDevice
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DX7VoicePacker.cpp
        Source/NeuralModelWrapper.cpp
        Source/MidiGenerator.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/model_data.h)

# Link libraries
target_link_libraries(ND7MidiDevice
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_devices
        juce::juce_gui_basics
        "${TORCH_LIBRARIES}"
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Disable JUCE networking features to avoid curl dependency
target_compile_definitions(ND7MidiDevice PRIVATE 
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

# Include directories
target_include_directories(ND7MidiDevice PRIVATE Source ${CMAKE_CURRENT_BINARY_DIR} ${GTK3_INCLUDE_DIRS} ${WEBKIT2GTK_INCLUDE_DIRS})

# Create binary resource from model file
function(create_binary_resource input_file output_file variable_name)
    add_custom_command(
        OUTPUT ${output_file}
        COMMAND xxd -i ${input_file} > ${output_file}
        DEPENDS ${input_file}
        COMMENT "Creating binary resource from ${input_file}"
    )
endfunction()

# Embed model as binary resource
create_binary_resource(
    ${CMAKE_CURRENT_SOURCE_DIR}/models/dx7_vae_model.pt
    ${CMAKE_CURRENT_BINARY_DIR}/model_data.h
    dx7_vae_model_pt
)