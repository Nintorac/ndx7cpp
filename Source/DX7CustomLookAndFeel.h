#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "UI/SevenSegmentLabel.h"

class DX7CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DX7CustomLookAndFeel();

    void drawLinearSlider(juce::Graphics& g,
                         int x, int y,
                         int width, int height,
                         float sliderPos,
                         float minSliderPos,
                         float maxSliderPos,
                         juce::Slider::SliderStyle style,
                         juce::Slider& slider) override;

    void drawLinearSliderThumb(juce::Graphics& g,
                              int x, int y,
                              int width, int height,
                              float sliderPos,
                              float minSliderPos,
                              float maxSliderPos,
                              juce::Slider::SliderStyle style,
                              juce::Slider& slider) override;

    int getSliderThumbRadius(juce::Slider& slider) override;

    juce::Label* createSliderTextBox(juce::Slider& slider) override;

private:
    juce::Image sliderTrackImage;
    juce::Image sliderKnobImage;
    juce::Image sevenSegBackgroundImage;
};
