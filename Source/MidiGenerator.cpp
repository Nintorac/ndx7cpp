#include "MidiGenerator.h"
#include "DX7BulkPacker.h"
#include <iostream>

MidiGenerator::MidiGenerator()
{
    if (!neuralModel.isModelLoaded()) {
        std::cerr << "Warning: Neural model failed to load\n";
    }
}

MidiGenerator::~MidiGenerator() = default;

void MidiGenerator::generateAndSendDX7Patches(const std::vector<float>& latentVector)
{
    if (!neuralModel.isModelLoaded()) {
        std::cerr << "Neural model not loaded\n";
        return;
    }
    
    auto voices = neuralModel.generateVoices(latentVector);
    if (voices.empty()) {
        std::cerr << "Failed to generate voices\n";
        return;
    }
    
    auto sysexData = DX7BulkPacker::packBulkDump(voices);
    if (sysexData.empty()) {
        std::cerr << "Failed to pack DX7 data\n";
        return;
    }
    
    sendSysExToMidiDevices(sysexData);
}

void MidiGenerator::generateAndSendRandomPatches()
{
    if (!neuralModel.isModelLoaded()) {
        std::cerr << "Neural model not loaded\n";
        return;
    }
    
    auto voices = neuralModel.generateRandomVoices();
    if (voices.empty()) {
        std::cerr << "Failed to generate random voices\n";
        return;
    }
    
    auto sysexData = DX7BulkPacker::packBulkDump(voices);
    if (sysexData.empty()) {
        std::cerr << "Failed to pack DX7 data\n";
        return;
    }
    
    sendSysExToMidiDevices(sysexData);
}

void MidiGenerator::sendSysExToMidiDevices(const std::vector<uint8_t>& sysexData)
{
    auto midiDevices = juce::MidiOutput::getAvailableDevices();
    
    if (midiDevices.isEmpty()) {
        std::cout << "No MIDI output devices available\n";
        return;
    }
    
    juce::MidiMessage sysexMessage = juce::MidiMessage::createSysExMessage(
        sysexData.data(), static_cast<int>(sysexData.size())
    );
    
    std::cout << "Sending DX7 bulk dump (" << sysexData.size() << " bytes) to MIDI devices:\n";
    
    for (int i = 0; i < midiDevices.size(); ++i) {
        auto device = juce::MidiOutput::openDevice(midiDevices[i].identifier);
        if (device) {
            device->sendMessageNow(sysexMessage);
            std::cout << "  - " << midiDevices[i].name << "\n";
        }
        else {
            std::cerr << "  - Failed to open: " << midiDevices[i].name << "\n";
        }
    }
}