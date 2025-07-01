#pragma once

#include <vector>
#include <cstdint>
#include "DX7Voice.h"

class DX7BulkPacker
{
public:
    static constexpr int N_VOICES = 32;
    static constexpr int BULK_DUMP_SIZE = 4096 + 6;
    
    static std::vector<uint8_t> packBulkDump(const std::vector<DX7Voice>& voices);
    static uint8_t calculateChecksum(const std::vector<uint8_t>& data);
    
private:
    static std::vector<uint8_t> packVoiceForBulk(const DX7Voice& voice);
};