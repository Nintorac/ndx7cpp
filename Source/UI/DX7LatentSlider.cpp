#include "DX7LatentSlider.h"
#include "AssetsData.h"

DX7LatentSlider::DX7LatentSlider(const juce::String& labelText, DX7LatentSliderLookAndFeel& lookAndFeel)
    : customLookAndFeel(lookAndFeel)
{
    // Load 7-segment background image
    backgroundImage = juce::ImageCache::getFromMemory(
        AssetsData::customise_7_seg_background_png,
        AssetsData::customise_7_seg_background_pngSize
    );

    // Setup label
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(label);

    // Setup slider
    slider.setRange(-3.0, 3.0, 0.01);
    slider.setValue(0.0);
    slider.setSliderStyle(juce::Slider::LinearVertical);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    slider.setLookAndFeel(&customLookAndFeel);
    addAndMakeVisible(slider);
}

DX7LatentSlider::~DX7LatentSlider()
{
    slider.setLookAndFeel(nullptr);
}

void DX7LatentSlider::paint(juce::Graphics& g)
{
    // Draw the 7-segment background image if available, matching slider width
    if (backgroundImage.isValid())
    {
        auto bounds = getLocalBounds();
        auto sliderWidth = slider.getWidth() + 10; // Match the width of the slider and its padding
        auto imageX = (bounds.getWidth() - sliderWidth) / 2.0f;

        g.drawImage(backgroundImage,
                   juce::Rectangle<float>(imageX, 0.0f, sliderWidth, static_cast<float>(bounds.getHeight())),
                   juce::RectanglePlacement::stretchToFit);
    }
}

void DX7LatentSlider::resized()
{
    auto bounds = getLocalBounds();

    // Create vertical flexbox: label at top, slider in middle (growing), textbox at bottom
    juce::FlexBox flexBox;
    flexBox.flexDirection = juce::FlexBox::Direction::column;
    flexBox.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    flexBox.alignItems = juce::FlexBox::AlignItems::center;

    // Label at top (fixed height)
    flexBox.items.add(juce::FlexItem(label)
        .withHeight(20.0f)
        .withWidth(60.0f)
    );

    // Slider in middle (flex grow to fill available space)
    // The slider's text box is positioned by JUCE based on setTextBoxStyle
    flexBox.items.add(juce::FlexItem(slider)
        .withFlex(1.0f)
        .withWidth(60.0f)
        .withMargin(juce::FlexItem::Margin(0, 10, 0, 10))
    );

    flexBox.performLayout(bounds.toFloat());
}
