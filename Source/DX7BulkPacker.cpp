#include "DX7BulkPacker.h"
#include "DX7VoicePacker.h"
#include <algorithm>
#include <numeric>
#include <iostream>

std::vector<uint8_t> DX7BulkPacker::packBulkDump(const std::vector<DX7Voice>& voices)
{
    if (voices.size() != N_VOICES) {
        return {};
    }
    
    std::vector<uint8_t> result;
    result.reserve(BULK_DUMP_SIZE);
    
    // SysEx header
    result.push_back(0xF0);
    result.push_back(0x43);  // Yamaha ID
    result.push_back(0x00);  // Sub-status & channel
    result.push_back(0x09);  // Format number (32 voices)
    result.push_back(0x20);  // Byte count MS
    result.push_back(0x00);  // Byte count LS
    
    // Pack all voices
    for (const auto& voice : voices) {
        auto voiceData = packVoiceForBulk(voice);
        if (voiceData.empty()) {
            return {};
        }
        result.insert(result.end(), voiceData.begin(), voiceData.end());
    }
    
    // Calculate and add checksum
    uint8_t checksum = calculateChecksum(std::vector<uint8_t>(result.begin() + 6, result.end()));
    result.push_back(checksum);
    
    // End SysEx
    result.push_back(0xF7);
    
    return result;
}

std::vector<uint8_t> DX7BulkPacker::packVoiceForBulk(const DX7Voice& voice)
{
    if (!voice.validate()) {
        std::cerr << "Voice validation failed in bulk packer" << std::endl;
        return {};
    }
    
    std::vector<uint8_t> result;
    result.reserve(128); // 128 bytes per voice in bulk dump
    
    const auto& oscillators = voice.getOscillators();
    const auto& global = voice.getGlobal();
    
    // Pack oscillators using DX7VoicePacker's helper functions
    for (const auto& osc : oscillators) {
        DX7VoicePacker::packOscillator(osc, result);
    }
    
    // Pack global parameters using DX7VoicePacker's helper function
    DX7VoicePacker::packGlobal(global, result);
    
    return result;
}

uint8_t DX7BulkPacker::calculateChecksum(const std::vector<uint8_t>& data)
{
    uint32_t sum = std::accumulate(data.begin(), data.end(), 0u);
    return (128 - (sum & 127)) % 128;
}