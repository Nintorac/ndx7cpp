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
    
    // Create buttons
    generateButton = std::make_unique<juce::TextButton>("Generate & Send");
    generateButton->addListener(this);
    addAndMakeVisible(*generateButton);
    
    randomizeButton = std::make_unique<juce::TextButton>("Randomize");
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
    
    // Sliders
    auto sliderArea = bounds.removeFromTop(250);
    int sliderWidth = sliderArea.getWidth() / NeuralModelWrapper::LATENT_DIM;
    
    for (int i = 0; i < NeuralModelWrapper::LATENT_DIM; ++i) {
        auto sliderBounds = sliderArea.removeFromLeft(sliderWidth).reduced(5);
        latentLabels[i]->setBounds(sliderBounds.removeFromTop(20));
        latentSliders[i]->setBounds(sliderBounds);
    }
    
    bounds.removeFromTop(20); // spacing
    
    // Buttons
    auto buttonArea = bounds.removeFromTop(40);
    int buttonWidth = buttonArea.getWidth() / 2;
    
    generateButton->setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(10));
    randomizeButton->setBounds(buttonArea.reduced(10));
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
    randomiseButton = std::make_unique<juce::TextButton>("Randomise");
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
    auto buttonBounds = bounds.withSizeKeepingCentre(200, 50);
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
        AssetsData::background_jpg,
        AssetsData::background_jpgSize
    );

    // Create title label
    titleLabel = std::make_unique<juce::Label>("title", "Neural DX7 Patch Generator");
    titleLabel->setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel->setJustificationType(juce::Justification::centred);
    titleLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(*titleLabel);

    // Create OPTIONS button
    optionsButton = std::make_unique<juce::ImageButton>("Options");
    auto optionsImage = juce::ImageCache::getFromMemory(
        AssetsData::global_options_png,
        AssetsData::global_options_pngSize
    );
    optionsButton->setImages(true, true, true,
                            optionsImage, 1.0f, juce::Colours::transparentBlack,
                            optionsImage, 0.8f, juce::Colours::transparentBlack,
                            optionsImage, 0.6f, juce::Colours::transparentBlack);
    addAndMakeVisible(*optionsButton);

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
    auto headerArea = bounds.removeFromTop(headerHeight);

    // OPTIONS button - 5% of total height, in top-left corner
    auto optionsHeight = static_cast<int>(bounds.getHeight() * 0.035f);
    auto optionsWidth = optionsHeight * 6; // Maintain aspect ratio (202:35 ≈ 5.77:1)
    optionsButton->setBounds(10, 10, optionsWidth, optionsHeight);

    // Title centered in remaining header area
    titleLabel->setBounds(headerArea.withTrimmedLeft(20).withTrimmedTop(10));

    // Tabbed component takes the rest of the space
    tabbedComponent->setBounds(bounds);
}