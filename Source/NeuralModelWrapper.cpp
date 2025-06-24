#include "NeuralModelWrapper.h"
#include <random>
#include <iostream>
#include <fstream>

NeuralModelWrapper::NeuralModelWrapper()
{
    // Don't load model in constructor to avoid crashes
    // Model will be loaded on first use
}

NeuralModelWrapper::~NeuralModelWrapper() = default;

bool NeuralModelWrapper::loadModelFromFile()
{
    if (modelLoaded) {
        return true; // Already loaded
    }
    
    try {
        // Try to load model from external file
        std::string modelPath = "models/dx7_vae_model.pt";
        
        // Check if file exists and is readable
        std::ifstream file(modelPath, std::ios::binary);
        if (!file.good()) {
            std::cerr << "Model file not found or not readable at: " << modelPath << "\n";
            return false;
        }
        file.close();
        
        // Load the model
        std::cout << "Loading neural model from: " << modelPath << "\n";
        model = torch::jit::load(modelPath);
        model.eval();
        modelLoaded = true;
        
        std::cout << "Neural model loaded successfully\n";
        return true;
    }
    catch (const c10::Error& e) {
        std::cerr << "PyTorch error loading model: " << e.what() << "\n";
        modelLoaded = false;
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load model from file: " << e.what() << "\n";
        modelLoaded = false;
        return false;
    }
    catch (...) {
        std::cerr << "Unknown error occurred while loading model\n";
        modelLoaded = false;
        return false;
    }
}

std::vector<DX7VoicePacker::Voice> NeuralModelWrapper::generateVoices(const std::vector<float>& latentVector)
{
    if (!modelLoaded && !loadModelFromFile()) {
        return {};
    }
    
    if (latentVector.size() != LATENT_DIM) {
        return {};
    }
    
    try {
        // Create tensor from latent vector
        torch::Tensor z = torch::tensor(latentVector).unsqueeze(0).repeat({N_VOICES, 1});
        
        // Generate parameters using the model
        std::vector<torch::jit::IValue> inputs;
        inputs.push_back(z);
        
        torch::Tensor logits = model.forward(inputs).toTensor();
        
        // Convert logits to parameters and then to voices
        std::vector<DX7VoicePacker::Voice> voices;
        voices.reserve(N_VOICES);
        
        for (int i = 0; i < N_VOICES; ++i) {
            torch::Tensor voiceLogits = logits[i];
            auto parameters = logitsToParameters(voiceLogits);
            voices.push_back(parametersToVoice(parameters));
        }
        
        return voices;
    }
    catch (const std::exception& e) {
        std::cerr << "Error generating voices: " << e.what() << "\n";
        return {};
    }
}

std::vector<DX7VoicePacker::Voice> NeuralModelWrapper::generateRandomVoices()
{
    if (!modelLoaded && !loadModelFromFile()) {
        return {};
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<float> dis(0.0f, 1.0f);
    
    std::vector<float> latentVector(LATENT_DIM);
    for (auto& val : latentVector) {
        val = dis(gen);
    }
    
    return generateVoices(latentVector);
}

std::vector<int> NeuralModelWrapper::logitsToParameters(const torch::Tensor& logits)
{
    // Apply argmax to get the most likely parameter values
    torch::Tensor paramTensor = logits.argmax(-1);
    
    // Convert to vector of ints
    std::vector<int> parameters;
    auto accessor = paramTensor.accessor<long, 1>();
    
    for (int i = 0; i < accessor.size(0); ++i) {
        parameters.push_back(static_cast<int>(accessor[i]));
    }
    
    return parameters;
}

DX7VoicePacker::Voice NeuralModelWrapper::parametersToVoice(const std::vector<int>& parameters)
{
    DX7VoicePacker::Voice voice;
    
    if (parameters.size() != 155) { // Expected DX7 parameter count
        return voice;
    }
    
    int paramIndex = 0;
    
    // Fill oscillator parameters (6 oscillators * 21 parameters each)
    for (int osc = 0; osc < 6; ++osc) {
        for (int param = 0; param < 21; ++param) {
            voice.oscillators[osc][param] = static_cast<uint8_t>(parameters[paramIndex++]);
        }
    }
    
    // Fill global parameters (29 parameters)
    for (int param = 0; param < 29; ++param) {
        voice.global[param] = static_cast<uint8_t>(parameters[paramIndex++]);
    }
    
    return voice;
}