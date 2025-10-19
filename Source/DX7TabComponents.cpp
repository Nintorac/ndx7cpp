#include "DX7TabComponents.h"
#include "AssetsData.h"

//==============================================================================
// DX7TabButton Implementation
//==============================================================================

DX7TabButton::DX7TabButton(const juce::String& name, juce::TabbedButtonBar& bar)
    : TabBarButton(name, bar)
{
    // Load the appropriate images based on tab name
    if (name == "Randomise")
    {
        selectedPressedImage = juce::ImageCache::getFromMemory(
            AssetsData::tab_randomise_selected_pressed_png,
            AssetsData::tab_randomise_selected_pressed_pngSize
        );
        selectedUnpressedImage = juce::ImageCache::getFromMemory(
            AssetsData::tab_randomise_selected_unpressed_png,
            AssetsData::tab_randomise_selected_unpressed_pngSize
        );
        unselectedPressedImage = juce::ImageCache::getFromMemory(
            AssetsData::tab_randomise_unselected_pressed_png,
            AssetsData::tab_randomise_unselected_pressed_pngSize
        );
        unselectedUnpressedImage = juce::ImageCache::getFromMemory(
            AssetsData::tab_randomise_unselected_unpressed_png,
            AssetsData::tab_randomise_unselected_unpressed_pngSize
        );
    }
    else if (name == "Customise")
    {
        selectedPressedImage = juce::ImageCache::getFromMemory(
            AssetsData::tab_customise_selected_pressed_png,
            AssetsData::tab_customise_selected_pressed_pngSize
        );
        selectedUnpressedImage = juce::ImageCache::getFromMemory(
            AssetsData::tab_customise_selected_unpressed_png,
            AssetsData::tab_customise_selected_unpressed_pngSize
        );
        unselectedPressedImage = juce::ImageCache::getFromMemory(
            AssetsData::tab_customise_unselected_pressed_png,
            AssetsData::tab_customise_unselected_pressed_pngSize
        );
        unselectedUnpressedImage = juce::ImageCache::getFromMemory(
            AssetsData::tab_customise_unselected_unpressed_png,
            AssetsData::tab_customise_unselected_unpressed_pngSize
        );
    }
}

void DX7TabButton::paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
    // Determine which image to use based on state
    juce::Image* imageToUse = nullptr;

    if (getToggleState()) // Selected tab
    {
        if (isButtonDown)
            imageToUse = &selectedPressedImage;
        else
            imageToUse = &selectedUnpressedImage;
    }
    else // Unselected tab
    {
        if (isButtonDown)
            imageToUse = &unselectedPressedImage;
        else
            imageToUse = &unselectedUnpressedImage;
    }

    // Draw the tab background image
    // The images already contain text, stretch to fit the tab bounds
    if (imageToUse != nullptr && imageToUse->isValid())
    {
        g.drawImage(*imageToUse,
                   getLocalBounds().toFloat(),
                   juce::RectanglePlacement::stretchToFit);
    }

}

int DX7TabButton::getBestTabLength(int depth)
{
    // Calculate width to fill available space evenly between tabs
    auto& bar = getTabbedButtonBar();
    auto numTabs = bar.getNumTabs();

    if (numTabs == 0)
        return 100;

    // Get total width
    auto totalWidth = bar.getWidth();

    // 88% for combined tabs, 4% left, 4% middle, 4% right = 12% padding total
    // So each tab gets 44% of total width (88% / 2)
    return static_cast<int>(totalWidth * 0.44f);
}

//==============================================================================
// DX7TabbedComponent Implementation
//==============================================================================

DX7TabbedComponent::DX7TabbedComponent()
    : TabbedComponent(juce::TabbedButtonBar::TabsAtTop)
{
}

void DX7TabbedComponent::resized()
{
    // Calculate tab bar depth based on aspect ratio of original images (734x85)
    // Each tab is 44% of total width, so calculate height to maintain aspect ratio
    auto tabWidth = getWidth() * 0.44f;
    auto aspectRatio = 85.0f / 734.0f;
    auto tabHeight = static_cast<int>(tabWidth * aspectRatio);

    setTabBarDepth(tabHeight);

    TabbedComponent::resized();
}

juce::TabBarButton* DX7TabbedComponent::createTabButton(const juce::String& tabName, int tabIndex)
{
    return new DX7TabButton(tabName, getTabbedButtonBar());
}
