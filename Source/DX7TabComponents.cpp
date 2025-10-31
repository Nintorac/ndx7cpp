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

    // Each tab is 33% of total width
    return static_cast<int>(totalWidth * 0.33f);
}

float DX7TabButton::getImageAspectRatio() const
{
    // Get aspect ratio from the loaded images (height / width)
    // Use selectedUnpressedImage as reference since all images should have same dimensions
    if (selectedUnpressedImage.isValid())
    {
        auto width = selectedUnpressedImage.getWidth();
        auto height = selectedUnpressedImage.getHeight();

        if (width > 0)
            return static_cast<float>(height) / static_cast<float>(width);
    }

    // Fallback to hardcoded aspect ratio if image not loaded
    return 85.0f / 734.0f;
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
    // Calculate tab bar depth based on aspect ratio from actual images
    // Each tab is 33% of total width, so calculate height to maintain aspect ratio
    auto tabWidth = getWidth() * 0.33f;

    // Get aspect ratio dynamically from first tab button if available
    float aspectRatio = 85.0f / 734.0f; // Fallback default

    if (getNumTabs() > 0)
    {
        if (auto* firstButton = dynamic_cast<DX7TabButton*>(getTabbedButtonBar().getTabButton(0)))
        {
            aspectRatio = firstButton->getImageAspectRatio();
        }
    }

    auto tabHeight = static_cast<int>(tabWidth * aspectRatio);

    setTabBarDepth(tabHeight);

    // Let JUCE do the initial layout
    TabbedComponent::resized();

    // Now manually position tabs with custom distribution
    // With 33% tabs, we have 34% space to distribute (100% - 66%)
    // Distribution: equal spacing on left, middle, and right
    auto& tabBar = getTabbedButtonBar();
    auto totalWidth = tabBar.getWidth();
    auto barHeight = tabBar.getHeight();
    auto numTabs = getNumTabs();

    if (numTabs > 0)
    {
        const float tabWidthPercent = 0.33f;
        const float totalSpacing = 1.0f - (tabWidthPercent * 2.0f); // 34% total spacing
        const float spacing = totalSpacing / 3.0f; // ~11.33% per gap (left, middle, right)

        auto tab1X = static_cast<int>(totalWidth * spacing);
        auto tab1Width = static_cast<int>(totalWidth * tabWidthPercent);
        auto tab2X = static_cast<int>(totalWidth * (spacing + tabWidthPercent + spacing));
        auto tab2Width = static_cast<int>(totalWidth * tabWidthPercent);

        // Position each tab manually
        for (int i = 0; i < numTabs; ++i)
        {
            if (auto* button = tabBar.getTabButton(i))
            {
                if (i == 0)
                {
                    button->setBounds(tab1X, 0, tab1Width, barHeight);
                }
                else if (i == 1)
                {
                    button->setBounds(tab2X, 0, tab2Width, barHeight);
                }
                // If more tabs are added, they would need additional positioning logic
            }
        }
    }
}

juce::TabBarButton* DX7TabbedComponent::createTabButton(const juce::String& tabName, int tabIndex)
{
    return new DX7TabButton(tabName, getTabbedButtonBar());
}
