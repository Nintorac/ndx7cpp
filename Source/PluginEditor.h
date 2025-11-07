#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/DX7CustomLookAndFeel.h"
#include "UI/DX7LatentSlider.h"
#include "UI/DX7TabComponents.h"

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

    DX7CustomLookAndFeel customLookAndFeel;

    std::vector<std::unique_ptr<DX7LatentSlider>> latentSliders;

    std::unique_ptr<juce::ImageButton> generateButton;
    std::unique_ptr<juce::ImageButton> randomizeButton;

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
    std::unique_ptr<juce::ImageButton> randomiseButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RandomiseTab)
};

class NeuralDX7PatchGeneratorEditor : public juce::AudioProcessorEditor
{
public:
    NeuralDX7PatchGeneratorEditor (NeuralDX7PatchGeneratorProcessor&);
    ~NeuralDX7PatchGeneratorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress& key) override;

private:
    NeuralDX7PatchGeneratorProcessor& audioProcessor;

    juce::Image headerImage;
    std::vector<juce::Image> backgroundImages;
    int currentBackgroundIndex;

    std::unique_ptr<DX7TabbedComponent> tabbedComponent;
    std::unique_ptr<CustomiseTab> customiseTab;
    std::unique_ptr<RandomiseTab> randomiseTab;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeuralDX7PatchGeneratorEditor)
};