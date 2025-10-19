#include "DX7CustomLookAndFeel.h"
#include "AssetsData.h"

DX7CustomLookAndFeel::DX7CustomLookAndFeel()
{
    // Load slider images from embedded assets
    sliderTrackImage = juce::ImageCache::getFromMemory(
        AssetsData::customise_slider_track_png,
        AssetsData::customise_slider_track_pngSize
    );

    sliderKnobImage = juce::ImageCache::getFromMemory(
        AssetsData::customise_slider_knob_png,
        AssetsData::customise_slider_knob_pngSize
    );

    // Set dark color scheme
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff3a2f2f));
    setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::trackColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff2a2020));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void DX7CustomLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                           int x, int y,
                                           int width, int height,
                                           float sliderPos,
                                           float minSliderPos,
                                           float maxSliderPos,
                                           juce::Slider::SliderStyle style,
                                           juce::Slider& slider)
{
    if (style == juce::Slider::LinearVertical && sliderTrackImage.isValid())
    {
        // Calculate track width and height maintaining aspect ratio
        auto imageAspect = static_cast<float>(sliderTrackImage.getWidth()) / sliderTrackImage.getHeight();
        auto trackWidth = width;
        auto trackHeight = static_cast<int>(trackWidth / imageAspect);

        // Center the track vertically if it's shorter than available height
        auto trackY = y + (height - trackHeight) / 2;

        auto trackBounds = juce::Rectangle<int>(x, trackY, trackWidth, trackHeight);

        // Draw the track image maintaining aspect ratio
        g.drawImage(sliderTrackImage,
                   trackBounds.toFloat(),
                   juce::RectanglePlacement::stretchToFit);

        // Now draw the thumb on top
        drawLinearSliderThumb(g, x, y, width, height, sliderPos,
                            minSliderPos, maxSliderPos, style, slider);
    }
    else
    {
        // Fallback to default rendering
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                        minSliderPos, maxSliderPos, style, slider);
    }
}

void DX7CustomLookAndFeel::drawLinearSliderThumb(juce::Graphics& g,
                                                int x, int y,
                                                int width, int height,
                                                float sliderPos,
                                                float minSliderPos,
                                                float maxSliderPos,
                                                juce::Slider::SliderStyle style,
                                                juce::Slider& slider)
{
    if (style == juce::Slider::LinearVertical && sliderKnobImage.isValid())
    {
        // Calculate knob dimensions - scale to half the width of the track
        auto targetKnobWidth = width / 2.0f;
        auto knobHeight = sliderKnobImage.getHeight() * (targetKnobWidth / sliderKnobImage.getWidth());

        // Align knob to the left edge (same as track)
        auto knobX = static_cast<float>(x);
        auto knobY = sliderPos - knobHeight / 2.0f;

        // Draw the knob image scaled to half width
        g.drawImage(sliderKnobImage,
                   juce::Rectangle<float>(knobX, knobY, targetKnobWidth, knobHeight),
                   juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        // Fallback to default rendering
        LookAndFeel_V4::drawLinearSliderThumb(g, x, y, width, height, sliderPos,
                                             minSliderPos, maxSliderPos, style, slider);
    }
}

int DX7CustomLookAndFeel::getSliderThumbRadius(juce::Slider& slider)
{
    // Return the height of the knob image
    if (sliderKnobImage.isValid())
        return sliderKnobImage.getHeight() / 2;

    return LookAndFeel_V4::getSliderThumbRadius(slider);
}
