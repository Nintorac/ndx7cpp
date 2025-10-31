#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "AssetsData.h"

//==============================================================================
// CustomiseTab Implementation
//==============================================================================

CustomiseTab::CustomiseTab(NeuralDX7PatchGeneratorProcessor& processor)
    : audioProcessor(processor)
{
    // Create latent dimension sliders
    latentSliders.resize(NeuralModelWrapper::LATENT_DIM);
    latentLabels.resize(NeuralModelWrapper::LATENT_DIM);
    
    for (int i = 0; i < NeuralModelWrapper::LATENT_DIM; ++i) {
        latentSliders[i] = std::make_unique<juce::Slider>();
        latentSliders[i]->setRange(-3.0, 3.0, 0.01);
        latentSliders[i]->setValue(0.0);
        latentSliders[i]->setSliderStyle(juce::Slider::LinearVertical);
        latentSliders[i]->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        latentSliders[i]->setLookAndFeel(&customLookAndFeel);
        latentSliders[i]->addListener(this);
        addAndMakeVisible(*latentSliders[i]);

        latentLabels[i] = std::make_unique<juce::Label>("label" + juce::String(i), "Z" + juce::String(i + 1));
        latentLabels[i]->setJustificationType(juce::Justification::centred);
        latentLabels[i]->setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(*latentLabels[i]);
    }
    
    // Create Generate button
    generateButton = std::make_unique<juce::ImageButton>("Generate");
    auto generateNormal = juce::ImageCache::getFromMemory(
        AssetsData::customise_generate_unpressed_png,
        AssetsData::customise_generate_unpressed_pngSize
    );
    auto generatePressed = juce::ImageCache::getFromMemory(
        AssetsData::customise_generate_pressed_png,
        AssetsData::customise_generate_pressed_pngSize
    );
    generateButton->setImages(true, true, true,
                             generateNormal, 1.0f, juce::Colours::transparentBlack,
                             generateNormal, 1.0f, juce::Colours::transparentBlack,
                             generatePressed, 1.0f, juce::Colours::transparentBlack);
    generateButton->addListener(this);
    addAndMakeVisible(*generateButton);

    // Create Randomize button
    randomizeButton = std::make_unique<juce::ImageButton>("Randomize");
    auto randomizeNormal = juce::ImageCache::getFromMemory(
        AssetsData::customise_randomise_unpressed_png,
        AssetsData::customise_randomise_unpressed_pngSize
    );
    auto randomizePressed = juce::ImageCache::getFromMemory(
        AssetsData::customise_randomise_pressed_png,
        AssetsData::customise_randomise_pressed_pngSize
    );
    randomizeButton->setImages(true, true, true,
                              randomizeNormal, 1.0f, juce::Colours::transparentBlack,
                              randomizeNormal, 1.0f, juce::Colours::transparentBlack,
                              randomizePressed, 1.0f, juce::Colours::transparentBlack);
    randomizeButton->addListener(this);
    addAndMakeVisible(*randomizeButton);
}

CustomiseTab::~CustomiseTab()
{
    // Reset LookAndFeel to avoid dangling pointers
    for (auto& slider : latentSliders)
    {
        if (slider)
            slider->setLookAndFeel(nullptr);
    }
}

void CustomiseTab::paint(juce::Graphics& g)
{
    // Don't fill background - let the main background image show through
}

void CustomiseTab::resized()
{
    auto bounds = getLocalBounds();

    bounds.removeFromTop(10); // spacing

    // Sliders area - use most of the vertical space
    auto sliderArea = bounds.removeFromTop(bounds.getHeight() - 80); // Reserve 80px for buttons
    int sliderWidth = sliderArea.getWidth() / NeuralModelWrapper::LATENT_DIM;

    for (int i = 0; i < NeuralModelWrapper::LATENT_DIM; ++i) {
        auto sliderBounds = sliderArea.removeFromLeft(sliderWidth).reduced(5);
        latentLabels[i]->setBounds(sliderBounds.removeFromTop(20));
        latentSliders[i]->setBounds(sliderBounds);
    }

    bounds.removeFromTop(10); // spacing between sliders and buttons

    // Buttons below the sliders - maintain aspect ratio (684:53 ≈ 12.9:1)
    int buttonWidth = bounds.getWidth() / 2;
    auto buttonHeight = static_cast<int>(buttonWidth / 12.9f);

    auto generateBounds = bounds.removeFromLeft(buttonWidth).reduced(10);
    generateButton->setBounds(generateBounds.withHeight(buttonHeight));

    auto randomizeBounds = bounds.reduced(10);
    randomizeButton->setBounds(randomizeBounds.withHeight(buttonHeight));
}

void CustomiseTab::sliderValueChanged(juce::Slider* slider)
{
    updateLatentValues();
}

void CustomiseTab::buttonClicked(juce::Button* button)
{
    if (button == generateButton.get()) {
        std::cout << "Generate & Send button clicked!" << std::endl;
        audioProcessor.generateAndSendMidi();
    }
    else if (button == randomizeButton.get()) {
        std::cout << "Randomize button clicked!" << std::endl;
        juce::Random random;
        for (auto& slider : latentSliders) {
            slider->setValue(random.nextFloat() * 6.0f - 3.0f); // Range -3 to 3
        }
        updateLatentValues();
    }
}

void CustomiseTab::updateLatentValues()
{
    std::vector<float> values;
    values.reserve(NeuralModelWrapper::LATENT_DIM);
    
    for (const auto& slider : latentSliders) {
        values.push_back(static_cast<float>(slider->getValue()));
    }
    
    audioProcessor.setLatentValues(values);
}

RandomiseTab::RandomiseTab(NeuralDX7PatchGeneratorProcessor& processor)
    : audioProcessor(processor)
{
    // Create cart image button
    randomiseButton = std::make_unique<juce::ImageButton>("Randomise");
    auto cartImage = juce::ImageCache::getFromMemory(
        AssetsData::randomise_cart_png,
        AssetsData::randomise_cart_pngSize
    );
    randomiseButton->setImages(true, true, true,
                              cartImage, 1.0f, juce::Colours::transparentBlack,
                              cartImage, 1.0f, juce::Colours::transparentBlack,
                              cartImage, 0.8f, juce::Colours::transparentBlack);
    randomiseButton->addListener(this);
    addAndMakeVisible(*randomiseButton);
}

RandomiseTab::~RandomiseTab()
{
}

void RandomiseTab::paint(juce::Graphics& g)
{
    // Don't fill background - let the main background image show through
}

void RandomiseTab::resized()
{
    auto bounds = getLocalBounds();

    // Cart image aspect ratio is 369:386 (roughly square, slightly taller)
    // Size it to fit nicely in the tab, centered
    auto maxSize = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.6f; // 60% of available space
    auto cartWidth = static_cast<int>(maxSize);
    auto cartHeight = static_cast<int>(maxSize * (386.0f / 369.0f)); // Maintain aspect ratio

    auto buttonBounds = bounds.withSizeKeepingCentre(cartWidth, cartHeight);
    randomiseButton->setBounds(buttonBounds);
}

void RandomiseTab::buttonClicked(juce::Button* button)
{
    if (button == randomiseButton.get()) {
        std::cout << "Randomise button clicked!" << std::endl;
        audioProcessor.generateRandomVoicesAndSend();
    }
}

NeuralDX7PatchGeneratorEditor::NeuralDX7PatchGeneratorEditor (NeuralDX7PatchGeneratorProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Load background image
    backgroundImage = juce::ImageCache::getFromMemory(
        AssetsData::background_png,
        AssetsData::background_pngSize
    );

    // Create tabs
    customiseTab = std::make_unique<CustomiseTab>(audioProcessor);
    randomiseTab = std::make_unique<RandomiseTab>(audioProcessor);

    // Create custom tabbed component
    tabbedComponent = std::make_unique<DX7TabbedComponent>();
    tabbedComponent->addTab("Randomise", juce::Colours::transparentBlack, randomiseTab.get(), false);
    tabbedComponent->addTab("Customise", juce::Colours::transparentBlack, customiseTab.get(), false);

    addAndMakeVisible(*tabbedComponent);

    // Call setSize() LAST after all components are initialized
    setSize (960, 540);
}

NeuralDX7PatchGeneratorEditor::~NeuralDX7PatchGeneratorEditor()
{
}

void NeuralDX7PatchGeneratorEditor::paint (juce::Graphics& g)
{
    // Draw background image
    if (backgroundImage.isValid())
    {
        g.drawImage(backgroundImage,
                   getLocalBounds().toFloat(),
                   juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        // Fallback to solid color if image fails to load
        g.fillAll(juce::Colour(0xff3a2f2f));
    }
}

void NeuralDX7PatchGeneratorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Reserve top 18% for header area
    auto headerHeight = static_cast<int>(bounds.getHeight() * 0.18f);
    bounds.removeFromTop(headerHeight);

    // Tabbed component takes the rest of the space
    tabbedComponent->setBounds(bounds);
}