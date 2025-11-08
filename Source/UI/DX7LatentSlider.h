#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "DX7LatentSliderLookAndFeel.h"

class DX7LatentSlider : public juce::Component
{
public:
    DX7LatentSlider(const juce::String& labelText, DX7LatentSliderLookAndFeel& lookAndFeel);
    ~DX7LatentSlider() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::Slider& getSlider() { return slider; }

private:
    DX7LatentSliderLookAndFeel& customLookAndFeel;
    juce::Image backgroundImage;

    juce::Label label;
    juce::Slider slider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DX7LatentSlider)
};
