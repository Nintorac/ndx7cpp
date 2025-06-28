# ND7 MIDI Device Makefile

BUILD_TYPE ?= Debug
# Detect OS
UNAME_S := $(shell uname -s)
ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
    LIBTORCH_PATH ?= C:/libtorch
    ifeq ($(BUILD_TYPE),Debug)
        LIBTORCH_URL := https://download.pytorch.org/libtorch/cpu/libtorch-win-shared-with-deps-debug-1.4.0.zip
    else
        LIBTORCH_URL := https://download.pytorch.org/libtorch/cpu/libtorch-win-shared-with-deps-1.4.0.zip
    endif
    UNZIP_CMD := unzip
    MOVE_CMD := mv
else
    DETECTED_OS := $(UNAME_S)
    LIBTORCH_PATH ?= /usr/local/libtorch
    LIBTORCH_URL := https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-static-with-deps-1.4.0%2Bcpu.zip
    UNZIP_CMD := unzip
    MOVE_CMD := sudo mv
endif

# Default paths - override with environment variables if needed
JUCE_PATH ?= ./JUCE
BUILD_DIR ?= build
MODEL_FILE ?= models/dx7_vae_model.pt

# Build configuration
CMAKE_FLAGS = -DCMAKE_PREFIX_PATH=$(LIBTORCH_PATH) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
ifeq ($(DETECTED_OS),Linux)
    CMAKE_FLAGS += -DCMAKE_FIND_LIBRARY_SUFFIXES=".a;.so"
    MAKE_FLAGS = -j$(shell nproc)
else ifeq ($(DETECTED_OS),Windows)
    CMAKE_FLAGS += -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
    MAKE_FLAGS = --parallel $(shell nproc)
else
    MAKE_FLAGS = -j$(shell sysctl -n hw.ncpu)
endif

.PHONY: all clean setup build install deps model help

# Default target
all: setup build

# Help target
help:
	@echo "ND7 MIDI Device Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Setup and build the project"
	@echo "  setup     - Initialize submodules and dependencies"
	@echo "  build     - Build the project"
	@echo "  clean     - Clean build directory"
	@echo "  install   - Install built plugins"
	@echo "  model     - Check for required model file"
	@echo "  deps      - Install system dependencies"
	@echo ""
	@echo "Environment Variables:"
	@echo "  LIBTORCH_PATH - Path to LibTorch installation (default: /usr/local/libtorch)"
	@echo "  JUCE_PATH     - Path to JUCE framework (default: ./JUCE)"
	@echo "  BUILD_DIR     - Build directory (default: build)"

# Setup JUCE submodule and check dependencies
setup: $(JUCE_PATH)/CMakeLists.txt model

$(JUCE_PATH)/CMakeLists.txt:
	@echo "Setting up JUCE submodule..."
	git submodule add https://github.com/juce-framework/JUCE.git JUCE || true
	git submodule update --init --recursive

# Check for model file
model:
	@if [ ! -f "$(MODEL_FILE)" ]; then \
		echo "ERROR: Model file $(MODEL_FILE) not found!"; \
		echo "Please follow the instructions in models/README.md to generate the model."; \
		echo "You can create a dummy file for testing: touch $(MODEL_FILE)"; \
		exit 1; \
	fi
	@echo "Model file found: $(MODEL_FILE)"

# Install system dependencies
deps:
ifeq ($(DETECTED_OS),Linux)
	@echo "Installing Linux system dependencies..."
	sudo apt-get update
	sudo apt-get install -y \
		build-essential \
		cmake \
		pkg-config \
		libasound2-dev \
		libjack-jackd2-dev \
		ladspa-sdk \
		libcurl4-openssl-dev \
		libfreetype6-dev \
		libx11-dev \
		libxcomposite-dev \
		libxcursor-dev \
		libxext-dev \
		libxinerama-dev \
		libxrandr-dev \
		libxrender-dev \
		libwebkit2gtk-4.1-dev \
		libglu1-mesa-dev \
		mesa-common-dev \
		libgtk-3-dev \
		libharfbuzz-dev
else ifeq ($(DETECTED_OS),Windows)
	@echo "Installing Windows dependencies via vcpkg..."
	vcpkg install zlib:x64-windows
else
	@echo "Dependencies not configured for this OS"
endif

# Configure with CMake
$(BUILD_DIR)/Makefile: CMakeLists.txt
	@echo "Configuring with CMake..."
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. $(CMAKE_FLAGS)

# Build the project
build: $(BUILD_DIR)/Makefile
	@echo "Building ND7 MIDI Device..."
ifeq ($(DETECTED_OS),Windows)
	cd $(BUILD_DIR) && cmake --build . --config $(BUILD_TYPE) $(MAKE_FLAGS)
else
	cd $(BUILD_DIR) && make $(MAKE_FLAGS)
endif
	@echo "Build complete! Binaries are in $(BUILD_DIR)/"

# Install built plugins
install: build
	@echo "Installing plugins..."
	cd $(BUILD_DIR) && make install

# Clean build directory
clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR)

# Download LibTorch (if not present)
download-libtorch:
	@if [ ! -d "$(LIBTORCH_PATH)" ]; then \
		echo "Downloading LibTorch for $(DETECTED_OS) ($(BUILD_TYPE) build)..."; \
		echo "URL: $(LIBTORCH_URL)"; \
		curl -L -o libtorch.zip $(LIBTORCH_URL); \
		$(UNZIP_CMD) libtorch.zip; \
		$(MOVE_CMD) libtorch $(LIBTORCH_PATH); \
		rm libtorch.zip; \
	else \
		echo "LibTorch already exists at $(LIBTORCH_PATH)"; \
	fi

# Create dummy model for testing
dummy-model:
	@echo "Creating dummy model file for testing..."
	mkdir -p models
	echo "dummy model content" > $(MODEL_FILE)
	@echo "WARNING: This is a dummy model file. Replace with actual trained model."

# Run the standalone application
run: build
	@echo "Running ND7 MIDI Device standalone..."
	$(BUILD_DIR)/ND7MidiDevice_artefacts/Standalone/ND7MidiDevice

# Package for distribution
package: build
	@echo "Creating distribution package..."
	tar -czf nd7-neural-dx7.tar.gz -C $(BUILD_DIR) NeuralDX7PatchGenerator_artefacts/
	@echo "Package created: nd7-neural-dx7.tar.gz"
	@echo "Libtorch libraries automatically included via RPATH"

