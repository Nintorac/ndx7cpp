#include "DX7VoicePacker.h"
#include <algorithm>
#include <numeric>

std::vector<uint8_t> DX7VoicePacker::packBulkDump(const std::vector<Voice>& voices)
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
        auto voiceData = packSingleVoice(voice);
        if (voiceData.empty()) {
            return {};
        }
        // Remove the SysEx wrapper for bulk dump
        result.insert(result.end(), voiceData.begin() + 6, voiceData.end() - 2);
    }
    
    // Calculate and add checksum
    uint8_t checksum = calculateChecksum(std::vector<uint8_t>(result.begin() + 6, result.end()));
    result.push_back(checksum);
    
    // End SysEx
    result.push_back(0xF7);
    
    return result;
}

std::vector<uint8_t> DX7VoicePacker::packSingleVoice(const Voice& voice)
{
    if (!validateParameters(voice)) {
        return {};
    }
    
    std::vector<uint8_t> result;
    result.reserve(VOICE_PARAM_COUNT + 8);
    
    // SysEx header for single voice
    result.push_back(0xF0);
    result.push_back(0x43);  // Yamaha ID
    result.push_back(0x00);  // Sub-status & channel
    result.push_back(0x00);  // Format number (1 voice)
    result.push_back(0x01);  // Byte count MS
    result.push_back(0x1B);  // Byte count LS (155 bytes)
    
    // Pack oscillators (6 oscillators * 17 bytes each = 102 bytes)
    for (const auto& osc : voice.oscillators) {
        packOscillator(osc, result);
    }
    
    // Pack global parameters (53 bytes)
    packGlobal(voice.global, result);
    
    // Calculate and add checksum
    uint8_t checksum = calculateChecksum(std::vector<uint8_t>(result.begin() + 6, result.end()));
    result.push_back(checksum);
    
    // End SysEx
    result.push_back(0xF7);
    
    return result;
}

void DX7VoicePacker::packOscillator(const std::array<uint8_t, 21>& osc, std::vector<uint8_t>& output)
{
    // Pack oscillator parameters according to DX7 format
    // R1-R4, L1-L4, BP, LD, RD (11 bytes)
    for (int i = 0; i < 11; ++i) {
        output.push_back(osc[i] & 0x7F);
    }
    
    // RC, LC combined (1 byte)
    output.push_back(((osc[11] & 0x03) << 2) | (osc[12] & 0x03));
    
    // DET, RS combined (1 byte)
    output.push_back(((osc[13] & 0x0F) << 3) | (osc[14] & 0x07));
    
    // KVS, AMS combined (1 byte)
    output.push_back(((osc[15] & 0x07) << 2) | (osc[16] & 0x03));
    
    // OL (1 byte)
    output.push_back(osc[17] & 0x7F);
    
    // FC, M combined (1 byte)
    output.push_back(((osc[18] & 0x1F) << 1) | (osc[19] & 0x01));
    
    // FF (1 byte)
    output.push_back(osc[20] & 0x7F);
}

void DX7VoicePacker::packGlobal(const std::array<uint8_t, 29>& global, std::vector<uint8_t>& output)
{
    // Pack global parameters according to DX7 format
    // PR1-PR4, PL1-PL4 (8 bytes)
    for (int i = 0; i < 8; ++i) {
        output.push_back(global[i] & 0x7F);
    }
    
    // ALG (1 byte)
    output.push_back(global[8] & 0x1F);
    
    // OKS, FB combined (1 byte)
    output.push_back(((global[9] & 0x01) << 3) | (global[10] & 0x07));
    
    // LFS, LFD, LPMD, LAMD (4 bytes)
    for (int i = 11; i < 15; ++i) {
        output.push_back(global[i] & 0x7F);
    }
    
    // LPMS, LFW, LKS combined (1 byte)
    output.push_back(((global[15] & 0x07) << 4) | ((global[16] & 0x07) << 1) | (global[17] & 0x01));
    
    // TRNSP (1 byte)
    output.push_back(global[18] & 0x3F);
    
    // NAME (10 bytes)
    for (int i = 19; i < 29; ++i) {
        output.push_back(global[i] & 0x7F);
    }
}

DX7VoicePacker::Voice DX7VoicePacker::unpackSingleVoice(const std::vector<uint8_t>& data)
{
    Voice voice;
    
    if (data.size() < VOICE_PARAM_COUNT + 8) {
        return voice;
    }
    
    const uint8_t* voiceData = data.data() + 6; // Skip SysEx header
    
    // Unpack oscillators
    for (int i = 0; i < N_OSC; ++i) {
        voice.oscillators[i] = unpackOscillator(voiceData + i * 17);
    }
    
    // Unpack global parameters
    voice.global = unpackGlobal(voiceData + N_OSC * 17);
    
    return voice;
}

std::array<uint8_t, 21> DX7VoicePacker::unpackOscillator(const uint8_t* data)
{
    std::array<uint8_t, 21> osc;
    
    // R1-R4, L1-L4, BP, LD, RD
    for (int i = 0; i < 11; ++i) {
        osc[i] = data[i];
    }
    
    // RC, LC
    osc[11] = (data[11] >> 2) & 0x03;
    osc[12] = data[11] & 0x03;
    
    // DET, RS
    osc[13] = (data[12] >> 3) & 0x0F;
    osc[14] = data[12] & 0x07;
    
    // KVS, AMS
    osc[15] = (data[13] >> 2) & 0x07;
    osc[16] = data[13] & 0x03;
    
    // OL
    osc[17] = data[14];
    
    // FC, M
    osc[18] = (data[15] >> 1) & 0x1F;
    osc[19] = data[15] & 0x01;
    
    // FF
    osc[20] = data[16];
    
    return osc;
}

std::array<uint8_t, 29> DX7VoicePacker::unpackGlobal(const uint8_t* data)
{
    std::array<uint8_t, 29> global;
    
    // PR1-PR4, PL1-PL4
    for (int i = 0; i < 8; ++i) {
        global[i] = data[i];
    }
    
    // ALG
    global[8] = data[8];
    
    // OKS, FB
    global[9] = (data[9] >> 3) & 0x01;
    global[10] = data[9] & 0x07;
    
    // LFS, LFD, LPMD, LAMD
    for (int i = 11; i < 15; ++i) {
        global[i] = data[i - 1];
    }
    
    // LPMS, LFW, LKS
    global[15] = (data[14] >> 4) & 0x07;
    global[16] = (data[14] >> 1) & 0x07;
    global[17] = data[14] & 0x01;
    
    // TRNSP
    global[18] = data[15];
    
    // NAME
    for (int i = 19; i < 29; ++i) {
        global[i] = data[i - 3];
    }
    
    return global;
}

bool DX7VoicePacker::validateParameters(const Voice& voice)
{
    // Validate oscillator parameters
    for (const auto& osc : voice.oscillators) {
        // R1-R4, L1-L4, BP, LD, RD, OL, FF (0-99)
        for (int i : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 17, 20}) {
            if (osc[i] > 99) return false;
        }
        
        // RC, LC (0-3)
        if (osc[11] > 3 || osc[12] > 3) return false;
        
        // DET (0-14)
        if (osc[13] > 14) return false;
        
        // RS (0-7)
        if (osc[14] > 7) return false;
        
        // KVS (0-7)
        if (osc[15] > 7) return false;
        
        // AMS (0-3)
        if (osc[16] > 3) return false;
        
        // FC (0-31)
        if (osc[18] > 31) return false;
        
        // M (0-1)
        if (osc[19] > 1) return false;
    }
    
    // Validate global parameters
    // PR1-PR4, PL1-PL4 (0-99)
    for (int i = 0; i < 8; ++i) {
        if (voice.global[i] > 99) return false;
    }
    
    // ALG (0-31)
    if (voice.global[8] > 31) return false;
    
    // OKS (0-1)
    if (voice.global[9] > 1) return false;
    
    // FB (0-7)
    if (voice.global[10] > 7) return false;
    
    // LFS, LFD, LPMD, LAMD (0-99)
    for (int i = 11; i < 15; ++i) {
        if (voice.global[i] > 99) return false;
    }
    
    // LPMS (0-7)
    if (voice.global[15] > 7) return false;
    
    // LFW (0-5)
    if (voice.global[16] > 5) return false;
    
    // LKS (0-1)
    if (voice.global[17] > 1) return false;
    
    // TRNSP (0-48)
    if (voice.global[18] > 48) return false;
    
    // NAME (0-127)
    for (int i = 19; i < 29; ++i) {
        if (voice.global[i] > 127) return false;
    }
    
    return true;
}

uint8_t DX7VoicePacker::calculateChecksum(const std::vector<uint8_t>& data)
{
    uint32_t sum = std::accumulate(data.begin(), data.end(), 0u);
    return (128 - (sum & 127)) % 128;
}