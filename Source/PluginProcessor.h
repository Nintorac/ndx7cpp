#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "NeuralModelWrapper.h"
#include "DX7VoicePacker.h"
#include "ThreadedInferenceEngine.h"

class NeuralDX7PatchGeneratorProcessor : public juce::AudioProcessor
{
public:
    NeuralDX7PatchGeneratorProcessor();
    ~NeuralDX7PatchGeneratorProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void generateAndSendMidi();
    void generateRandomVoicesAndSend();
    void setLatentValues(const std::vector<float>& values);
    void debouncedPreGeneration(); // For slider changes

private:
    std::unique_ptr<ThreadedInferenceEngine> inferenceEngine;
    std::vector<float> latentVector;
    juce::Random random;
    juce::MidiBuffer pendingMidiMessages;
    
    // Debouncing for slider changes
    std::unique_ptr<juce::Timer> debounceTimer;
    
    void addMidiSysEx(const std::vector<uint8_t>& sysexData);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeuralDX7PatchGeneratorProcessor)
};