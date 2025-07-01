#include "DX7VoicePacker.h"
#include <algorithm>
#include <numeric>
#include <iostream>


std::vector<uint8_t> DX7VoicePacker::packSingleVoice(const DX7Voice& voice)
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
    
    // Pack oscillators in single voice format (6 oscillators * 21 bytes each = 126 bytes)
    for (const auto& osc : voice.getOscillators()) {
        packSingleVoiceOscillator(osc, result);
    }
    
    // Pack global parameters in single voice format (29 bytes)
    packSingleVoiceGlobal(voice.getGlobal(), result);
    
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

DX7Voice DX7VoicePacker::unpackSingleVoice(const std::vector<uint8_t>& data)
{
    std::array<std::array<uint8_t, 21>, N_OSC> oscillators;
    std::array<uint8_t, 29> global;
    
    // Initialize with zeros
    for (auto& osc : oscillators) {
        osc.fill(0);
    }
    global.fill(0);
    
    if (data.size() < VOICE_PARAM_COUNT + 8) {
        return DX7Voice(oscillators, global);
    }
    
    const uint8_t* voiceData = data.data() + 6; // Skip SysEx header
    
    // Unpack oscillators
    for (int i = 0; i < N_OSC; ++i) {
        oscillators[i] = unpackOscillator(voiceData + i * 17);
    }
    
    // Unpack global parameters
    global = unpackGlobal(voiceData + N_OSC * 17);
    
    return DX7Voice(oscillators, global);
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

bool DX7VoicePacker::validateParameters(const DX7Voice& voice)
{
    // Use DX7Voice's built-in validation
    bool isValid = voice.validate();
    if (!isValid) {
        std::cerr << "Voice validation failed" << std::endl;
        // Debug: print some parameter values
        const auto& oscs = voice.getOscillators();
        const auto& global = voice.getGlobal();
        std::cerr << "Sample osc[0] params: ";
        for (int i = 0; i < 5; ++i) {
            std::cerr << (int)oscs[0][i] << " ";
        }
        std::cerr << "\nSample global params: ";
        for (int i = 0; i < 5; ++i) {
            std::cerr << (int)global[i] << " ";
        }
        std::cerr << std::endl;
    }
    return isValid;
}

uint8_t DX7VoicePacker::calculateChecksum(const std::vector<uint8_t>& data)
{
    uint32_t sum = std::accumulate(data.begin(), data.end(), 0u);
    return (128 - (sum & 127)) % 128;
}

void DX7VoicePacker::packSingleVoiceOscillator(const std::array<uint8_t, 21>& osc, std::vector<uint8_t>& output)
{
    // Convert from bulk dump order to single voice dump order
    // Bulk dump order: R1,R2,R3,R4,L1,L2,L3,L4,BP,LD,RD,RC,LC,DET,RS,KVS,AMS,OL,FC,M,FF
    // Single voice order: R1,R2,R3,R4,L1,L2,L3,L4,BP,LD,RD,LC,RC,RS,AMS,KVS,OL,M,FC,FF,DET
    
    // R1-R4, L1-L4, BP, LD, RD (positions 0-10 same in both)
    for (int i = 0; i < 11; ++i) {
        output.push_back(osc[i] & 0x7F);
    }
    
    // LC, RC (swapped: bulk has RC,LC at 11,12, single voice has LC,RC at 11,12)
    output.push_back(osc[12] & 0x7F);  // LC (bulk pos 12 -> single pos 11)
    output.push_back(osc[11] & 0x7F);  // RC (bulk pos 11 -> single pos 12)
    
    // RS (bulk pos 14 -> single pos 13)
    output.push_back(osc[14] & 0x7F);
    
    // AMS (bulk pos 16 -> single pos 14)
    output.push_back(osc[16] & 0x7F);
    
    // KVS (bulk pos 15 -> single pos 15)
    output.push_back(osc[15] & 0x7F);
    
    // OL (bulk pos 17 -> single pos 16)
    output.push_back(osc[17] & 0x7F);
    
    // M (bulk pos 19 -> single pos 17)
    output.push_back(osc[19] & 0x7F);
    
    // FC (bulk pos 18 -> single pos 18)
    output.push_back(osc[18] & 0x7F);
    
    // FF (bulk pos 20 -> single pos 19)
    output.push_back(osc[20] & 0x7F);
    
    // DET (bulk pos 13 -> single pos 20)
    output.push_back(osc[13] & 0x7F);
}

void DX7VoicePacker::packSingleVoiceGlobal(const std::array<uint8_t, 29>& global, std::vector<uint8_t>& output)
{
    // Global parameters have the same order in both bulk dump and single voice dump
    // So we can just copy them directly
    for (int i = 0; i < 29; ++i) {
        output.push_back(global[i] & 0x7F);
    }
}