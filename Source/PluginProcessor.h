#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "NeuralModelWrapper.h"
#include "DX7VoicePacker.h"

class ND7MidiDeviceProcessor : public juce::AudioProcessor
{
public:
    ND7MidiDeviceProcessor();
    ~ND7MidiDeviceProcessor() override;

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
    void setLatentValues(const std::vector<float>& values);

private:
    NeuralModelWrapper neuralModel;
    std::vector<float> latentVector;
    juce::Random random;
    
    void sendMidiSysEx(const std::vector<uint8_t>& sysexData);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ND7MidiDeviceProcessor)
};