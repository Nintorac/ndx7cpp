#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class DX7TabButton : public juce::TabBarButton
{
public:
    DX7TabButton(const juce::String& name, juce::TabbedButtonBar& bar);

    void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override;

    int getBestTabLength(int depth) override;

    float getImageAspectRatio() const;

private:
    juce::Image selectedPressedImage;
    juce::Image selectedUnpressedImage;
    juce::Image unselectedPressedImage;
    juce::Image unselectedUnpressedImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DX7TabButton)
};

class DX7TabbedComponent : public juce::TabbedComponent
{
public:
    DX7TabbedComponent();

    juce::TabBarButton* createTabButton(const juce::String& tabName, int tabIndex) override;

    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DX7TabbedComponent)
};
