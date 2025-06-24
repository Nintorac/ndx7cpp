#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "NeuralModelWrapper.h"
#include "DX7VoicePacker.h"

class MidiGenerator
{
public:
    MidiGenerator();
    ~MidiGenerator();
    
    void generateAndSendDX7Patches(const std::vector<float>& latentVector);
    void generateAndSendRandomPatches();
    
    bool isModelReady() const { return neuralModel.isModelLoaded(); }
    
private:
    NeuralModelWrapper neuralModel;
    
    void sendSysExToMidiDevices(const std::vector<uint8_t>& sysexData);
};