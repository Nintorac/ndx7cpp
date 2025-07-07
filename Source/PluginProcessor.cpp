#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DX7BulkPacker.h"
#include "DX7VoicePacker.h"

// Simple debounce timer class
class DebounceTimer : public juce::Timer
{
public:
    DebounceTimer(std::function<void()> callback) : callback_(callback) {}
    
    void timerCallback() override
    {
        stopTimer();
        if (callback_)
            callback_();
    }
    
    void debounce(int milliseconds)
    {
        startTimer(milliseconds);
    }
    
private:
    std::function<void()> callback_;
};


NeuralDX7PatchGeneratorProcessor::NeuralDX7PatchGeneratorProcessor()
     : AudioProcessor (BusesProperties())
{
    latentVector.resize(NeuralModelWrapper::LATENT_DIM, 0.0f);
    inferenceEngine = std::make_unique<ThreadedInferenceEngine>();
    inferenceEngine->startInferenceThread();
    
    // Create debounce timer for slider changes
    debounceTimer = std::make_unique<DebounceTimer>([this]() {
        // Pre-generate voice for current latent vector
        inferenceEngine->preGenerateCustomVoice(latentVector);
    });
}

NeuralDX7PatchGeneratorProcessor::~NeuralDX7PatchGeneratorProcessor()
{
    if (inferenceEngine)
    {
        inferenceEngine->stopInferenceThread();
    }
}

const juce::String NeuralDX7PatchGeneratorProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NeuralDX7PatchGeneratorProcessor::acceptsMidi() const
{
    return false;
}

bool NeuralDX7PatchGeneratorProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool NeuralDX7PatchGeneratorProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double NeuralDX7PatchGeneratorProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NeuralDX7PatchGeneratorProcessor::getNumPrograms()
{
    return 1;
}

int NeuralDX7PatchGeneratorProcessor::getCurrentProgram()
{
    return 0;
}

void NeuralDX7PatchGeneratorProcessor::setCurrentProgram (int index)
{
}

const juce::String NeuralDX7PatchGeneratorProcessor::getProgramName (int index)
{
    return {};
}

void NeuralDX7PatchGeneratorProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void NeuralDX7PatchGeneratorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
}

void NeuralDX7PatchGeneratorProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NeuralDX7PatchGeneratorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void NeuralDX7PatchGeneratorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    // Add any pending MIDI messages to the output
    if (!pendingMidiMessages.isEmpty()) {
        std::cout << "Processing " << pendingMidiMessages.getNumEvents() << " pending MIDI messages" << std::endl;
        midiMessages.addEvents(pendingMidiMessages, 0, buffer.getNumSamples(), 0);
        pendingMidiMessages.clear();
        std::cout << "MIDI messages added to output buffer" << std::endl;
    }
}

bool NeuralDX7PatchGeneratorProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* NeuralDX7PatchGeneratorProcessor::createEditor()
{
    return new NeuralDX7PatchGeneratorEditor (*this);
}

void NeuralDX7PatchGeneratorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream(destData, true);
    
    for (float value : latentVector) {
        stream.writeFloat(value);
    }
}

void NeuralDX7PatchGeneratorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);
    
    for (int i = 0; i < NeuralModelWrapper::LATENT_DIM && !stream.isExhausted(); ++i) {
        latentVector[i] = stream.readFloat();
    }
}

void NeuralDX7PatchGeneratorProcessor::generateAndSendMidi()
{
    std::cout << "generateAndSendMidi() called" << std::endl;
    
    if (!inferenceEngine->isModelLoaded()) {
        std::cout << "Neural model not loaded yet, request ignored" << std::endl;
        return;
    }
    
    std::cout << "Generating voice with latent vector: [";
    for (size_t i = 0; i < latentVector.size(); ++i) {
        std::cout << latentVector[i];
        if (i < latentVector.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
    
    // Use cached request for instant response if available
    inferenceEngine->requestCachedCustomVoice(latentVector, [this](std::optional<DX7Voice> voiceOpt) {
        if (!voiceOpt.has_value()) {
            std::cout << "Warning: Attempted to send null voice - ignoring request" << std::endl;
            return;
        }
        
        std::cout << "Got custom voice, sending as single voice SysEx" << std::endl;
        
        // For customise functionality, send as single voice SysEx
        auto sysexData = DX7VoicePacker::packSingleVoice(voiceOpt.value());
        if (!sysexData.empty()) {
            std::cout << "Packed single voice SysEx data: " << sysexData.size() << " bytes" << std::endl;
            addMidiSysEx(sysexData);
        } else {
            std::cout << "Failed to pack single voice SysEx data!" << std::endl;
        }
    });
}

void NeuralDX7PatchGeneratorProcessor::generateRandomVoicesAndSend()
{
    std::cout << "generateRandomVoicesAndSend() called" << std::endl;
    
    // Try to use buffered voices first for instant response
    if (inferenceEngine->hasBufferedRandomVoices()) {
        std::cout << "Using buffered random voices for instant response" << std::endl;
        auto voices = inferenceEngine->getBufferedRandomVoices();
        
        if (!voices.empty()) {
            std::cout << "Got " << voices.size() << " buffered voices, packing into SysEx..." << std::endl;
            auto sysexData = DX7BulkPacker::packBulkDump(voices);
            
            if (!sysexData.empty()) {
                std::cout << "SysEx data packed successfully, sending..." << std::endl;
                addMidiSysEx(sysexData);
            } else {
                std::cout << "Failed to pack SysEx data!" << std::endl;
            }
            return;
        }
    }
    
    // If no buffer available, do nothing to prevent weird behavior on rapid clicks
    std::cout << "No buffered voices available, ignoring request" << std::endl;
}

void NeuralDX7PatchGeneratorProcessor::setLatentValues(const std::vector<float>& values)
{
    if (values.size() == NeuralModelWrapper::LATENT_DIM) {
        latentVector = values;
        // Trigger debounced pre-generation
        debouncedPreGeneration();
    }
}

void NeuralDX7PatchGeneratorProcessor::debouncedPreGeneration()
{
    // Debounce slider changes with 150ms delay
    if (debounceTimer) {
        static_cast<DebounceTimer*>(debounceTimer.get())->debounce(150);
    }
}

void NeuralDX7PatchGeneratorProcessor::addMidiSysEx(const std::vector<uint8_t>& sysexData)
{
    std::cout << "addMidiSysEx() called with " << sysexData.size() << " bytes" << std::endl;
    
#if JucePlugin_Build_LV2 || JucePlugin_Build_VST3
    // For LV2 and VST3, strip SysEx start (0xF0) and end (0xF7) bytes
    if (sysexData.size() >= 2 && sysexData[0] == 0xF0 && sysexData[sysexData.size() - 1] == 0xF7)
    {
        std::cout << "LV2/VST3 plugin detected - stripping SysEx start/end bytes" << std::endl;
        std::vector<uint8_t> strippedData(sysexData.begin() + 1, sysexData.end() - 1);
        
        juce::MidiMessage sysexMessage = juce::MidiMessage::createSysExMessage(
            strippedData.data(), static_cast<int>(strippedData.size())
        );
        
        pendingMidiMessages.addEvent(sysexMessage, 0);
        std::cout << "Added stripped SysEx message to pending MIDI buffer" << std::endl;
    }
    else
    {
        std::cout << "SysEx data doesn't have expected start/end bytes, sending as-is" << std::endl;
        juce::MidiMessage sysexMessage = juce::MidiMessage::createSysExMessage(
            sysexData.data(), static_cast<int>(sysexData.size())
        );
        
        pendingMidiMessages.addEvent(sysexMessage, 0);
        std::cout << "Added SysEx message to pending MIDI buffer" << std::endl;
    }
#else
    // For other plugin formats, send SysEx data as-is
    juce::MidiMessage sysexMessage = juce::MidiMessage::createSysExMessage(
        sysexData.data(), static_cast<int>(sysexData.size())
    );
    
    pendingMidiMessages.addEvent(sysexMessage, 0);
    std::cout << "Added SysEx message to pending MIDI buffer" << std::endl;
#endif
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NeuralDX7PatchGeneratorProcessor();
}