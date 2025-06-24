# ND7 MIDI Device Makefile

# Default paths - override with environment variables if needed
LIBTORCH_PATH ?= /usr/local/libtorch
JUCE_PATH ?= ./JUCE
BUILD_DIR ?= build
MODEL_FILE ?= models/dx7_vae_model.pt

# Build configuration
CMAKE_FLAGS = -DCMAKE_PREFIX_PATH=$(LIBTORCH_PATH) -DCMAKE_BUILD_TYPE=Release
MAKE_FLAGS = -j$(shell nproc)

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

# Install system dependencies (Ubuntu/Debian)
deps:
	@echo "Installing system dependencies..."
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
		libgtk-3-dev

# Configure with CMake
$(BUILD_DIR)/Makefile: CMakeLists.txt
	@echo "Configuring with CMake..."
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. $(CMAKE_FLAGS)

# Build the project
build: $(BUILD_DIR)/Makefile
	@echo "Building ND7 MIDI Device..."
	cd $(BUILD_DIR) && make $(MAKE_FLAGS)
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
		echo "Downloading LibTorch..."; \
		wget https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-1.4.0%2Bcpu.zip; \
		unzip libtorch-*.zip; \
		sudo mv libtorch $(LIBTORCH_PATH); \
		rm libtorch-*.zip; \
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
	mkdir -p dist
	cp -r $(BUILD_DIR)/ND7MidiDevice_artefacts/* dist/
	tar -czf nd7-midi-device.tar.gz dist/
	@echo "Package created: nd7-midi-device.tar.gz"

