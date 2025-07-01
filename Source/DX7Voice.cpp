#include "DX7Voice.h"
#include <torch/torch.h>
#include <iostream>

DX7Voice::DX7Voice(const std::array<std::array<uint8_t, 21>, N_OSC>& oscillators, 
                   const std::array<uint8_t, 29>& global)
    : oscillators(oscillators), global(global)
{
}

const std::array<std::array<uint8_t, 21>, DX7Voice::N_OSC>& DX7Voice::getOscillators() const
{
    return oscillators;
}

const std::array<uint8_t, 29>& DX7Voice::getGlobal() const
{
    return global;
}

void DX7Voice::setOscillators(const std::array<std::array<uint8_t, 21>, N_OSC>& oscillators)
{
    this->oscillators = oscillators;
}

void DX7Voice::setGlobal(const std::array<uint8_t, 29>& global)
{
    this->global = global;
}

DX7Voice DX7Voice::fromParameters(const std::vector<int>& parameters)
{
    std::array<std::array<uint8_t, 21>, N_OSC> oscillators;
    std::array<uint8_t, 29> global;
    
    // Initialize with zeros
    for (auto& osc : oscillators) {
        osc.fill(0);
    }
    global.fill(0);
    
    if (parameters.size() != 155) { // Expected DX7 parameter count
        return DX7Voice(oscillators, global);
    }
    
    int paramIndex = 0;
    
    // Use bulk dump ordering as canonical - no mapping needed
    // Bulk dump osc order: R1,R2,R3,R4,L1,L2,L3,L4,BP,LD,RD,RC,LC,DET,RS,KVS,AMS,OL,FC,M,FF
    for (int osc = 0; osc < 6; ++osc) {
        for (int param = 0; param < 21; ++param) {
            oscillators[osc][param] = static_cast<uint8_t>(parameters[paramIndex++]);
        }
    }
    
    // Bulk dump global order: PR1,PR2,PR3,PR4,PL1,PL2,PL3,PL4,ALG,OKS,FB,LFS,LFD,LPMD,LAMD,LPMS,LFW,LKS,TRNSP,NAME1-10
    for (int param = 0; param < 29; ++param) {
        global[param] = static_cast<uint8_t>(parameters[paramIndex++]);
    }
    
    return DX7Voice(oscillators, global);
}

std::vector<int> DX7Voice::logitsToParameters(const torch::Tensor& logits)
{
    // Apply argmax to get the most likely parameter values
    torch::Tensor paramTensor = logits.argmax(-1);
    
    // Convert to vector of ints
    std::vector<int> parameters;
#ifdef _WIN32
    // Windows: Convert to int32 to avoid linker issues with long type
    paramTensor = paramTensor.to(torch::kInt32);
    auto accessor = paramTensor.accessor<int, 1>();
#else
    // Linux: Use long type (works fine)
    auto accessor = paramTensor.accessor<long, 1>();
#endif
    
    for (int i = 0; i < accessor.size(0); ++i) {
        parameters.push_back(static_cast<int>(accessor[i]));
    }
    
    return parameters;
}

bool DX7Voice::validate() const
{
    // Validate oscillator parameters using bulk dump ordering
    // Bulk dump osc order: R1,R2,R3,R4,L1,L2,L3,L4,BP,LD,RD,RC,LC,DET,RS,KVS,AMS,OL,FC,M,FF
    for (int oscIdx = 0; oscIdx < N_OSC; ++oscIdx) {
        const auto& osc = oscillators[oscIdx];
        // R1-R4, L1-L4 (0-99)
        for (int i = 0; i < 8; ++i) {
            if (osc[i] > 99) {
                std::cerr << "Osc " << oscIdx << " R/L param " << i << " = " << (int)osc[i] << " > 99" << std::endl;
                return false;
            }
        }
        // BP, LD, RD (0-99)
        for (int i = 8; i < 11; ++i) {
            if (osc[i] > 99) {
                std::cerr << "Osc " << oscIdx << " BP/LD/RD param " << i << " = " << (int)osc[i] << " > 99" << std::endl;
                return false;
            }
        }
        // RC, LC (0-3)
        if (osc[11] > 3) {
            std::cerr << "Osc " << oscIdx << " RC = " << (int)osc[11] << " > 3" << std::endl;
            return false;
        }
        if (osc[12] > 3) {
            std::cerr << "Osc " << oscIdx << " LC = " << (int)osc[12] << " > 3" << std::endl;
            return false;
        }
        // DET (0-14)
        if (osc[13] > 14) {
            std::cerr << "Osc " << oscIdx << " DET = " << (int)osc[13] << " > 14" << std::endl;
            return false;
        }
        // RS (0-7)
        if (osc[14] > 7) {
            std::cerr << "Osc " << oscIdx << " RS = " << (int)osc[14] << " > 7" << std::endl;
            return false;
        }
        // KVS (0-7)
        if (osc[15] > 7) {
            std::cerr << "Osc " << oscIdx << " KVS = " << (int)osc[15] << " > 7" << std::endl;
            return false;
        }
        // AMS (0-3)
        if (osc[16] > 3) {
            std::cerr << "Osc " << oscIdx << " AMS = " << (int)osc[16] << " > 3" << std::endl;
            return false;
        }
        // OL (0-99)
        if (osc[17] > 99) {
            std::cerr << "Osc " << oscIdx << " OL = " << (int)osc[17] << " > 99" << std::endl;
            return false;
        }
        // FC (0-31)
        if (osc[18] > 31) {
            std::cerr << "Osc " << oscIdx << " FC = " << (int)osc[18] << " > 31" << std::endl;
            return false;
        }
        // M (0-1)
        if (osc[19] > 1) {
            std::cerr << "Osc " << oscIdx << " M = " << (int)osc[19] << " > 1" << std::endl;
            return false;
        }
        // FF (0-99)
        if (osc[20] > 99) {
            std::cerr << "Osc " << oscIdx << " FF = " << (int)osc[20] << " > 99" << std::endl;
            return false;
        }
    }
    
    // Validate global parameters using bulk dump ordering
    // Bulk dump global order: PR1,PR2,PR3,PR4,PL1,PL2,PL3,PL4,ALG,OKS,FB,LFS,LFD,LPMD,LAMD,LPMS,LFW,LKS,TRNSP,NAME1-10
    
    // PR1-PR4, PL1-PL4 (0-99)
    for (int i = 0; i < 8; ++i) {
        if (global[i] > 99) {
            std::cerr << "Global PR/PL param " << i << " = " << (int)global[i] << " > 99" << std::endl;
            return false;
        }
    }
    // ALG (0-31)
    if (global[8] > 31) {
        std::cerr << "ALG = " << (int)global[8] << " > 31" << std::endl;
        return false;
    }
    // OKS (0-1)
    if (global[9] > 1) {
        std::cerr << "OKS = " << (int)global[9] << " > 1" << std::endl;
        return false;
    }
    // FB (0-7)
    if (global[10] > 7) {
        std::cerr << "FB = " << (int)global[10] << " > 7" << std::endl;
        return false;
    }
    // LFS, LFD, LPMD, LAMD (0-99)
    for (int i = 11; i < 15; ++i) {
        if (global[i] > 99) {
            std::cerr << "Global LF param " << i << " = " << (int)global[i] << " > 99" << std::endl;
            return false;
        }
    }
    // LPMS (0-7)
    if (global[15] > 7) {
        std::cerr << "LPMS = " << (int)global[15] << " > 7" << std::endl;
        return false;
    }
    // LFW (0-5)
    if (global[16] > 5) {
        std::cerr << "LFW = " << (int)global[16] << " > 5" << std::endl;
        return false;
    }
    // LKS (0-1)
    if (global[17] > 1) {
        std::cerr << "LKS = " << (int)global[17] << " > 1" << std::endl;
        return false;
    }
    // TRNSP (0-48)
    if (global[18] > 48) {
        std::cerr << "TRNSP = " << (int)global[18] << " > 48" << std::endl;
        return false;
    }
    // Voice name chars (ASCII, skip validation)
    
    return true;
}