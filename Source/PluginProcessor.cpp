#include "PluginProcessor.h"
#include "PluginEditor.h"

ND7MidiDeviceProcessor::ND7MidiDeviceProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
{
    latentVector.resize(NeuralModelWrapper::LATENT_DIM, 0.0f);
}

ND7MidiDeviceProcessor::~ND7MidiDeviceProcessor()
{
}

const juce::String ND7MidiDeviceProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ND7MidiDeviceProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ND7MidiDeviceProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ND7MidiDeviceProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ND7MidiDeviceProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ND7MidiDeviceProcessor::getNumPrograms()
{
    return 1;
}

int ND7MidiDeviceProcessor::getCurrentProgram()
{
    return 0;
}

void ND7MidiDeviceProcessor::setCurrentProgram (int index)
{
}

const juce::String ND7MidiDeviceProcessor::getProgramName (int index)
{
    return {};
}

void ND7MidiDeviceProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void ND7MidiDeviceProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
}

void ND7MidiDeviceProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ND7MidiDeviceProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void ND7MidiDeviceProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
}

bool ND7MidiDeviceProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ND7MidiDeviceProcessor::createEditor()
{
    return new ND7MidiDeviceEditor (*this);
}

void ND7MidiDeviceProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream(destData, true);
    
    for (float value : latentVector) {
        stream.writeFloat(value);
    }
}

void ND7MidiDeviceProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);
    
    for (int i = 0; i < NeuralModelWrapper::LATENT_DIM && !stream.isExhausted(); ++i) {
        latentVector[i] = stream.readFloat();
    }
}

void ND7MidiDeviceProcessor::generateAndSendMidi()
{
    std::cout << "generateAndSendMidi() called" << std::endl;
    
    if (!neuralModel.isModelLoaded()) {
        std::cout << "Neural model not loaded, attempting to load..." << std::endl;
        if (!neuralModel.loadModelFromFile()) {
            std::cout << "Failed to load neural model!" << std::endl;
            return;
        }
        std::cout << "Neural model loaded successfully!" << std::endl;
    }
    
    std::cout << "Generating voices with latent vector: [";
    for (size_t i = 0; i < latentVector.size(); ++i) {
        std::cout << latentVector[i];
        if (i < latentVector.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
    
    auto voices = neuralModel.generateVoices(latentVector);
    if (voices.empty()) {
        std::cout << "No voices generated!" << std::endl;
        return;
    }
    
    std::cout << "Generated " << voices.size() << " voices" << std::endl;
    
    auto sysexData = DX7VoicePacker::packBulkDump(voices);
    if (!sysexData.empty()) {
        std::cout << "Packed SysEx data: " << sysexData.size() << " bytes" << std::endl;
        sendMidiSysEx(sysexData);
    } else {
        std::cout << "Failed to pack SysEx data!" << std::endl;
    }
}

void ND7MidiDeviceProcessor::setLatentValues(const std::vector<float>& values)
{
    if (values.size() == NeuralModelWrapper::LATENT_DIM) {
        latentVector = values;
    }
}

void ND7MidiDeviceProcessor::sendMidiSysEx(const std::vector<uint8_t>& sysexData)
{
    std::cout << "sendMidiSysEx() called with " << sysexData.size() << " bytes" << std::endl;
    
    juce::MidiMessage sysexMessage = juce::MidiMessage::createSysExMessage(
        sysexData.data(), static_cast<int>(sysexData.size())
    );
    
    std::cout << "Created MIDI SysEx message" << std::endl;
    
    // Send to all connected MIDI outputs
    auto devices = juce::MidiOutput::getAvailableDevices();
    std::cout << "Found " << devices.size() << " MIDI output devices:" << std::endl;
    
    for (int i = 0; i < devices.size(); ++i) {
        std::cout << "  Device " << i << ": " << devices[i].name << std::endl;
        auto device = juce::MidiOutput::openDevice(devices[i].identifier);
        if (device) {
            std::cout << "  Successfully opened device: " << devices[i].name << std::endl;
            device->sendMessageNow(sysexMessage);
            std::cout << "  Sent SysEx message to: " << devices[i].name << std::endl;
        } else {
            std::cout << "  Failed to open device: " << devices[i].name << std::endl;
        }
    }
    
    if (devices.size() == 0) {
        std::cout << "No MIDI output devices available!" << std::endl;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ND7MidiDeviceProcessor();
}