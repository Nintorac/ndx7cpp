#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include "DX7Voice.h"

class DX7VoicePacker
{
public:
    static constexpr int N_OSC = 6;
    static constexpr int VOICE_PARAM_COUNT = 155;
    
    enum class ParameterType {
        R1, R2, R3, R4, L1, L2, L3, L4, BP, LD, RD, RC, LC, 
        DET, RS, KVS, AMS, OL, FC, M, FF,
        PR1, PR2, PR3, PR4, PL1, PL2, PL3, PL4, ALG, OKS, FB,
        LFS, LFD, LPMD, LAMD, LPMS, LFW, LKS, TRNSP,
        NAME_CHAR_1, NAME_CHAR_2, NAME_CHAR_3, NAME_CHAR_4, NAME_CHAR_5,
        NAME_CHAR_6, NAME_CHAR_7, NAME_CHAR_8, NAME_CHAR_9, NAME_CHAR_10
    };
    
    static std::vector<uint8_t> packSingleVoice(const DX7Voice& voice);
    static DX7Voice unpackSingleVoice(const std::vector<uint8_t>& data);
    
    static bool validateParameters(const DX7Voice& voice);
    static uint8_t calculateChecksum(const std::vector<uint8_t>& data);
    
    // Public helper functions for use by DX7BulkPacker
    static void packOscillator(const std::array<uint8_t, 21>& osc, std::vector<uint8_t>& output);
    static void packGlobal(const std::array<uint8_t, 29>& global, std::vector<uint8_t>& output);
    
    // Single voice unpacked format functions
    static void packSingleVoiceOscillator(const std::array<uint8_t, 21>& osc, std::vector<uint8_t>& output);
    static void packSingleVoiceGlobal(const std::array<uint8_t, 29>& global, std::vector<uint8_t>& output);
    
private:
    static std::array<uint8_t, 21> unpackOscillator(const uint8_t* data);
    static std::array<uint8_t, 29> unpackGlobal(const uint8_t* data);
};