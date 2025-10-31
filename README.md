# ND7 MIDI Device

> [!NOTE]  
> This repo almost entirely slop/vibe code since I know almost nothing of C++


A JUCE plugin that exposes the neural DX7 model as a MIDI device, generating DX7 SysEx patches using the trained neural network.

## Features

- Neural DX7 patch generation using libtorch
- Real-time MIDI SysEx output
- Interactive latent space control
- Embedded model (no external files needed)
- Supports VST3, AU, and Standalone formats

## Building

### Prerequisites

- CMake 3.15+
- C++17 compatible compiler
- LibTorch (PyTorch C++ library)
- JUCE framework (included as submodule)

### Quick Start

```bash
# Install system dependencies (Ubuntu/Debian)
make deps

# Setup JUCE and dependencies
make setup

# Create a dummy model file for testing
make dummy-model

# Build the project
make build

# Run standalone version
make run
```

### Manual Setup

1. Clone JUCE as a submodule:
   ```bash
   git submodule add https://github.com/juce-framework/JUCE.git JUCE
   git submodule update --init --recursive
   ```

2. Install LibTorch:
   - Download from https://pytorch.org/get-started/locally/
   - Extract and set CMAKE_PREFIX_PATH to the LibTorch directory

3. Generate the model file:
   - Follow instructions in `models/README.md`
   - Place `dx7_vae_model.pt` in the `models/` directory

### Build Commands

```bash
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/libtorch
make -j8
```

## Usage

1. Load the plugin in your DAW or run the standalone version
2. Use the 8 sliders to control the neural model's latent space
3. Click "Generate & Send" to send DX7 patches via MIDI SysEx
4. Click "Randomize" to set random latent values
5. Connect to a DX7, Dexed, or other compatible FM synthesizer

## Makefile Targets

- `make all` - Setup and build the project
- `make setup` - Initialize submodules and dependencies
- `make build` - Build the project
- `make clean` - Clean build directory
- `make install` - Install built plugins
- `make model` - Check for required model file
- `make deps` - Install system dependencies
- `make dummy-model` - Create dummy model for testing
- `make run` - Run standalone application
- `make package` - Create distribution package

## Architecture

- **DX7VoicePacker**: Handles DX7 SysEx format encoding/decoding
- **NeuralModelWrapper**: Manages libtorch model inference
- **MidiGenerator**: Handles MIDI output and device management
- **PluginProcessor/Editor**: JUCE plugin interface

The neural model is embedded as binary data in the executable, so no external model files are needed at runtime.

## Model Generation

To use a real trained model instead of the dummy:

1. Train the model using the Python code in the parent directory:
   ```bash
   cd ../projects/dx7_vae
   python experiment.py
   ```

2. Export to TorchScript format:
   ```python
   import torch
   from agoge import InferenceWorker

   # Load your trained model
   model = InferenceWorker('hasty-copper-dogfish', 'dx7-vae', with_data=False).model

   # Convert to TorchScript
   scripted_model = torch.jit.script(model)

   # Save the scripted model
   scripted_model.save('dx7_vae_model.pt')
   ```

3. Replace the dummy model file with the real one.

## Acknowledgements

This project uses the **DSEG7 Classic** font by keshikan for the 7-segment LED display styling:
- Font: DSEG7 Classic Regular
- Author: keshikan
- Website: https://www.keshikan.net/fonts-e.html
- GitHub: https://github.com/keshikan/DSEG
- License: SIL Open Font License 1.1

The DSEG font family provides authentic 7-segment and 14-segment display typefaces for digital display aesthetics.