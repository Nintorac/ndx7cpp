#include "PluginProcessor.h"
#include "PluginEditor.h"

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
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    g.setColour(juce::Colours::white);
    g.setFont(12.0f);
    
    g.drawText("Control the neural model's latent space to generate DX7 patches", 
               10, getHeight() - 30, getWidth() - 20, 20, juce::Justification::centred);
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
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
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
    // Create title label
    titleLabel = std::make_unique<juce::Label>("title", "NeuralDX7 Patch Generator");
    titleLabel->setFont(juce::Font(20.0f, juce::Font::bold));
    titleLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*titleLabel);

    // Create tabs
    customiseTab = std::make_unique<CustomiseTab>(audioProcessor);
    randomiseTab = std::make_unique<RandomiseTab>(audioProcessor);

    // Create custom tabbed component
    tabbedComponent = std::make_unique<DX7TabbedComponent>();
    tabbedComponent->addTab("Randomise", juce::Colours::lightgrey, randomiseTab.get(), false);
    tabbedComponent->addTab("Customise", juce::Colours::lightgrey, customiseTab.get(), false);

    addAndMakeVisible(*tabbedComponent);

    // Call setSize() LAST after all components are initialized
    setSize (600, 400);
}

NeuralDX7PatchGeneratorEditor::~NeuralDX7PatchGeneratorEditor()
{
}

void NeuralDX7PatchGeneratorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void NeuralDX7PatchGeneratorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel->setBounds(bounds.removeFromTop(40).reduced(10));
    
    bounds.removeFromTop(5); // spacing
    
    // Tabbed component takes the rest of the space
    tabbedComponent->setBounds(bounds);
}