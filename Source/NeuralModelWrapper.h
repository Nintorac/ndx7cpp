#pragma once

#include <torch/torch.h>
#include <torch/script.h>
#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include "DX7VoicePacker.h"

class NeuralModelWrapper
{
public:
    static constexpr int LATENT_DIM = 8;
    static constexpr int N_VOICES = 32;
    
    NeuralModelWrapper();
    ~NeuralModelWrapper();
    
    bool loadModelFromFile();
    std::vector<DX7VoicePacker::Voice> generateVoices(const std::vector<float>& latentVector);
    std::vector<DX7VoicePacker::Voice> generateRandomVoices();
    std::vector<DX7VoicePacker::Voice> generateMultipleRandomVoices();
    
    bool isModelLoaded() const { return modelLoaded; }
    
private:
    torch::jit::script::Module model;
    bool modelLoaded = false;
    
    std::vector<int> logitsToParameters(const torch::Tensor& logits);
    DX7VoicePacker::Voice parametersToVoice(const std::vector<int>& parameters);
};