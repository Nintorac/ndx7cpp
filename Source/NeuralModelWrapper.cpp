#include "NeuralModelWrapper.h"
#include "EmbeddedModelLoader.h"
#include "DX7Voice.h"
#include <random>
#include <iostream>
#include <fstream>
#include <sstream>

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
        // First try to load from embedded compressed model
        std::cout << "Loading neural model from embedded data\n";
        auto modelData = EmbeddedModelLoader::loadCompressedModel();
        
        // Create a stringstream from the decompressed data
        std::stringstream modelStream(std::string(modelData.begin(), modelData.end()));
        
        // Load model from the stream
        model = torch::jit::load(modelStream);
        model.eval();
        modelLoaded = true;
        
        std::cout << "Neural model loaded successfully from embedded data (" 
                  << modelData.size() << " bytes)\n";
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load embedded model: " << e.what() << "\n";
        
        // Fallback to external file
        try {
            std::string modelPath = "models/dx7_vae_model.pt";
            
            // Check if file exists and is readable
            std::ifstream file(modelPath, std::ios::binary);
            if (!file.good()) {
                std::cerr << "Model file not found or not readable at: " << modelPath << "\n";
                return false;
            }
            file.close();
            
            // Load the model
            std::cout << "Loading neural model from fallback file: " << modelPath << "\n";
            model = torch::jit::load(modelPath);
            model.eval();
            modelLoaded = true;
            
            std::cout << "Neural model loaded successfully from file\n";
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
}

std::vector<DX7Voice> NeuralModelWrapper::generateVoices(const std::vector<float>& latentVector)
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
        std::vector<DX7Voice> voices;
        voices.reserve(N_VOICES);
        
        for (int i = 0; i < N_VOICES; ++i) {
            torch::Tensor voiceLogits = logits[i];
            auto parameters = DX7Voice::logitsToParameters(voiceLogits);
            voices.push_back(DX7Voice::fromParameters(parameters));
        }
        
        return voices;
    }
    catch (const std::exception& e) {
        std::cerr << "Error generating voices: " << e.what() << "\n";
        return {};
    }
}

std::vector<DX7Voice> NeuralModelWrapper::generateRandomVoices()
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

std::vector<DX7Voice> NeuralModelWrapper::generateMultipleRandomVoices()
{
    if (!modelLoaded && !loadModelFromFile()) {
        return {};
    }
    
    try {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dis(0.0f, 1.0f);
        
        // Create tensor with different random latent vectors for each voice
        torch::Tensor z = torch::randn({N_VOICES, LATENT_DIM});
        
        // Generate parameters using the model
        std::vector<torch::jit::IValue> inputs;
        inputs.push_back(z);
        
        torch::Tensor logits = model.forward(inputs).toTensor();
        
        // Convert logits to parameters and then to voices
        std::vector<DX7Voice> voices;
        voices.reserve(N_VOICES);
        
        for (int i = 0; i < N_VOICES; ++i) {
            torch::Tensor voiceLogits = logits[i];
            auto parameters = DX7Voice::logitsToParameters(voiceLogits);
            voices.push_back(DX7Voice::fromParameters(parameters));
        }
        
        return voices;
    }
    catch (const std::exception& e) {
        std::cerr << "Error generating multiple random voices: " << e.what() << "\n";
        return {};
    }
}

