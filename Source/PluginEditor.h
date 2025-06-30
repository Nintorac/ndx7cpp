#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class FullWidthLookAndFeel : public juce::LookAndFeel_V4
{
public:
    int getTabButtonBestWidth(juce::TabBarButton& button, int tabDepth) override
    {
        return button.getTabbedButtonBar().getWidth() / button.getTabbedButtonBar().getNumTabs();
    }
};

class CustomiseTab : public juce::Component,
                     public juce::Slider::Listener,
                     public juce::Button::Listener
{
public:
    CustomiseTab(NeuralDX7PatchGeneratorProcessor& processor);
    ~CustomiseTab() override;

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
    
    void updateLatentValues();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomiseTab)
};

class RandomiseTab : public juce::Component,
                     public juce::Button::Listener
{
public:
    RandomiseTab(NeuralDX7PatchGeneratorProcessor& processor);
    ~RandomiseTab() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    
    void buttonClicked (juce::Button* button) override;

private:
    NeuralDX7PatchGeneratorProcessor& audioProcessor;
    std::unique_ptr<juce::TextButton> randomiseButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RandomiseTab)
};

class NeuralDX7PatchGeneratorEditor : public juce::AudioProcessorEditor
{
public:
    NeuralDX7PatchGeneratorEditor (NeuralDX7PatchGeneratorProcessor&);
    ~NeuralDX7PatchGeneratorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    NeuralDX7PatchGeneratorProcessor& audioProcessor;
    
    FullWidthLookAndFeel customLookAndFeel;
    std::unique_ptr<juce::Label> titleLabel;
    std::unique_ptr<juce::TabbedComponent> tabbedComponent;
    std::unique_ptr<CustomiseTab> customiseTab;
    std::unique_ptr<RandomiseTab> randomiseTab;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeuralDX7PatchGeneratorEditor)
};