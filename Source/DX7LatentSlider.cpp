#include "DX7LatentSlider.h"

DX7LatentSlider::DX7LatentSlider(const juce::String& labelText, DX7CustomLookAndFeel& lookAndFeel)
    : customLookAndFeel(lookAndFeel)
{
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
    // Don't fill background - let the main background image show through
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
        .withWidth(60.0f));

    // Slider in middle (flex grow to fill available space)
    // The slider's text box is positioned by JUCE based on setTextBoxStyle
    flexBox.items.add(juce::FlexItem(slider)
        .withFlex(1.0f)
        .withWidth(60.0f)
        .withMargin(juce::FlexItem::Margin(5, 0, 0, 0)));

    flexBox.performLayout(bounds.toFloat());
}
