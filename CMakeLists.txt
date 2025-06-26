cmake_minimum_required(VERSION 3.15)

project(NeuralDX7PatchGenerator VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find GTK3 and webkit2gtk
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(WEBKIT2GTK REQUIRED webkit2gtk-4.1)

# Add JUCE
add_subdirectory(JUCE)

# Configure for static linking
set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
set(BUILD_SHARED_LIBS OFF)

# Add libtorch
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

# Try to use static libraries where available, fall back to shared
set(TORCH_STATIC_LIBRARIES)
foreach(lib IN ITEMS torch c10 mkldnn protobuf onnx gloo fbgemm cpuinfo clog pthreadpool nnpack pytorch_qnnpack qnnpack protobuf-lite onnx_proto caffe2_protos)
    find_library(${lib}_STATIC_LIB lib${lib}.a PATHS ${CMAKE_PREFIX_PATH}/lib NO_DEFAULT_PATH)
    if(${lib}_STATIC_LIB)
        list(APPEND TORCH_STATIC_LIBRARIES ${${lib}_STATIC_LIB})
    endif()
endforeach()

# Static linking flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

# Set RPATH for remaining dynamic dependencies
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/lib")
set(CMAKE_BUILD_RPATH "$ORIGIN:$ORIGIN/lib")
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Add zlib for decompression
find_package(ZLIB REQUIRED)

# Find harfbuzz for text rendering
find_package(PkgConfig REQUIRED)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)

# Create the plugin target
juce_add_plugin(NeuralDX7PatchGenerator
    COMPANY_NAME "NintoracAudio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_PLUGIN TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE NinA
    PLUGIN_CODE NeuD
    FORMATS Standalone
    PRODUCT_NAME "NeuralDX7PatchGenerator")

# Override standalone binary location to be in Release/ instead of Release/Standalone/
set_target_properties(NeuralDX7PatchGenerator_Standalone PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/NeuralDX7PatchGenerator_artefacts/Release")

# Add source files
target_sources(NeuralDX7PatchGenerator
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DX7VoicePacker.cpp
        Source/NeuralModelWrapper.cpp
        Source/MidiGenerator.cpp
        Source/EmbeddedModelLoader.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/model_data.h)

# Link libraries
target_link_libraries(NeuralDX7PatchGenerator
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_devices
        juce::juce_gui_basics
        ${TORCH_STATIC_LIBRARIES}
        "${TORCH_LIBRARIES}"
        ZLIB::ZLIB
        ${HARFBUZZ_LIBRARIES}
        ${CMAKE_DL_LIBS}
        pthread
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Set RPATH to look for libraries in lib/ subdirectory
set_target_properties(NeuralDX7PatchGenerator PROPERTIES
    BUILD_RPATH "$ORIGIN/lib"
    INSTALL_RPATH "$ORIGIN/lib")

# Copy libtorch libraries to lib subfolder after build
add_custom_command(TARGET NeuralDX7PatchGenerator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_CURRENT_BINARY_DIR}/NeuralDX7PatchGenerator_artefacts/Release/lib"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_PREFIX_PATH}/lib/libtorch.so"
        "${CMAKE_CURRENT_BINARY_DIR}/NeuralDX7PatchGenerator_artefacts/Release/lib/libtorch.so"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_PREFIX_PATH}/lib/libc10.so"
        "${CMAKE_CURRENT_BINARY_DIR}/NeuralDX7PatchGenerator_artefacts/Release/lib/libc10.so"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_PREFIX_PATH}/lib/libgomp-753e6e92.so.1"
        "${CMAKE_CURRENT_BINARY_DIR}/NeuralDX7PatchGenerator_artefacts/Release/lib/libgomp-753e6e92.so.1"
    COMMENT "Copying libtorch libraries to Release/lib directory")

# Disable JUCE networking features to avoid curl dependency
target_compile_definitions(NeuralDX7PatchGenerator PRIVATE 
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

# Include directories
target_include_directories(NeuralDX7PatchGenerator PRIVATE Source ${CMAKE_CURRENT_BINARY_DIR} ${GTK3_INCLUDE_DIRS} ${WEBKIT2GTK_INCLUDE_DIRS})

# Create compressed binary resource from model file
function(create_compressed_binary_resource input_file output_file variable_name)
    get_filename_component(input_basename ${input_file} NAME)
    set(compressed_file ${CMAKE_CURRENT_BINARY_DIR}/${input_basename}.gz)
    
    add_custom_command(
        OUTPUT ${compressed_file}
        COMMAND gzip -9 -c ${input_file} > ${compressed_file}
        DEPENDS ${input_file}
        COMMENT "Compressing ${input_file}"
    )
    
    add_custom_command(
        OUTPUT ${output_file}
        COMMAND cd ${CMAKE_CURRENT_BINARY_DIR} && xxd -i ${input_basename}.gz > model_data.h
        DEPENDS ${compressed_file}
        COMMENT "Creating compressed binary resource from ${compressed_file}"
    )
endfunction()

# Embed compressed model as binary resource
create_compressed_binary_resource(
    ${CMAKE_CURRENT_SOURCE_DIR}/models/dx7_vae_model.pt
    ${CMAKE_CURRENT_BINARY_DIR}/model_data.h
    dx7_vae_model_pt_gz
)