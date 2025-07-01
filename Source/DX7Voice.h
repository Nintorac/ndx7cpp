#pragma once

#include <vector>
#include <array>
#include <cstdint>

// Forward declaration
namespace at { class Tensor; }
namespace torch { using Tensor = at::Tensor; }

class DX7Voice
{
public:
    static constexpr int N_OSC = 6;
    
    DX7Voice(const std::array<std::array<uint8_t, 21>, N_OSC>& oscillators, 
             const std::array<uint8_t, 29>& global);
    
    const std::array<std::array<uint8_t, 21>, N_OSC>& getOscillators() const;
    const std::array<uint8_t, 29>& getGlobal() const;
    
    void setOscillators(const std::array<std::array<uint8_t, 21>, N_OSC>& oscillators);
    void setGlobal(const std::array<uint8_t, 29>& global);
    
    static DX7Voice fromParameters(const std::vector<int>& parameters);
    static std::vector<int> logitsToParameters(const torch::Tensor& logits);
    
    bool validate() const;
    
private:
    std::array<std::array<uint8_t, 21>, N_OSC> oscillators;
    std::array<uint8_t, 29> global;
};