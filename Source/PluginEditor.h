#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class NeuralDX7PatchGeneratorEditor : public juce::AudioProcessorEditor,
                            public juce::Slider::Listener,
                            public juce::Button::Listener
{
public:
    NeuralDX7PatchGeneratorEditor (NeuralDX7PatchGeneratorProcessor&);
    ~NeuralDX7PatchGeneratorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    
    void sliderValueChanged (juce::Slider* slider) override;
    void buttonClicked (juce::Button* button) override;

private:
    NeuralDX7PatchGeneratorProcessor& audioProcessor;
    
    std::vector<std::unique_ptr<juce::Slider>> latentSliders;
    std::vector<std::unique_ptr<juce::Label>> latentLabels;
    
    std::unique_ptr<juce::TextButton> generateButton;
    std::unique_ptr<juce::TextButton> randomizeButton;
    std::unique_ptr<juce::Label> titleLabel;
    
    void updateLatentValues();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeuralDX7PatchGeneratorEditor)
};