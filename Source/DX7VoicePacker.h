#pragma once

#include <vector>
#include <array>
#include <cstdint>

class DX7VoicePacker
{
public:
    static constexpr int N_VOICES = 32;
    static constexpr int N_OSC = 6;
    static constexpr int VOICE_PARAM_COUNT = 155;
    static constexpr int BULK_DUMP_SIZE = 4096 + 6;
    
    enum class ParameterType {
        R1, R2, R3, R4, L1, L2, L3, L4, BP, LD, RD, RC, LC, 
        DET, RS, KVS, AMS, OL, FC, M, FF,
        PR1, PR2, PR3, PR4, PL1, PL2, PL3, PL4, ALG, OKS, FB,
        LFS, LFD, LPMD, LAMD, LPMS, LFW, LKS, TRNSP,
        NAME_CHAR_1, NAME_CHAR_2, NAME_CHAR_3, NAME_CHAR_4, NAME_CHAR_5,
        NAME_CHAR_6, NAME_CHAR_7, NAME_CHAR_8, NAME_CHAR_9, NAME_CHAR_10
    };
    
    struct Voice {
        std::array<std::array<uint8_t, 21>, N_OSC> oscillators;
        std::array<uint8_t, 29> global;
    };
    
    static std::vector<uint8_t> packBulkDump(const std::vector<Voice>& voices);
    static std::vector<uint8_t> packSingleVoice(const Voice& voice);
    static Voice unpackSingleVoice(const std::vector<uint8_t>& data);
    
    static bool validateParameters(const Voice& voice);
    static uint8_t calculateChecksum(const std::vector<uint8_t>& data);
    
private:
    static void packOscillator(const std::array<uint8_t, 21>& osc, std::vector<uint8_t>& output);
    static void packGlobal(const std::array<uint8_t, 29>& global, std::vector<uint8_t>& output);
    static std::array<uint8_t, 21> unpackOscillator(const uint8_t* data);
    static std::array<uint8_t, 29> unpackGlobal(const uint8_t* data);
};