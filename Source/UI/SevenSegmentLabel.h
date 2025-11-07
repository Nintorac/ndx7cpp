#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Custom label for 7-segment display with background image
class SevenSegmentLabel : public juce::Label
{
public:
    SevenSegmentLabel(const juce::Image& backgroundImg, const void* fontData, size_t fontDataSize)
        : backgroundImage(backgroundImg)
    {
        // Configure as editable slider textbox
        setJustificationType(juce::Justification::centredRight);
        setKeyboardType(juce::TextInputTarget::decimalKeyboard);
        setEditable(false, false, false);

        // Load and apply the 7-segment display font with bold styling
        auto typeface = juce::Typeface::createSystemTypefaceFor(fontData, fontDataSize);
        setFont(juce::Font(typeface).withHeight(14.0f).withStyle(juce::Font::bold));

        // Style for LED display appearance
        setColour(juce::Label::textColourId, juce::Colour(0xff940034)); // Red LED
        setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack); // Transparent - image drawn in paint()
        setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    }

    void paint(juce::Graphics& g) override
    {
        // Draw the background image
        if (backgroundImage.isValid())
        {
            g.drawImage(backgroundImage,
                       getLocalBounds().toFloat(),
                       juce::RectanglePlacement::stretchToFit);
        }

        // Let the parent class draw the text on top
        juce::Label::paint(g);
    }

private:
    juce::Image backgroundImage;
};
