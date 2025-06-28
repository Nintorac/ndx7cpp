#include "EmbeddedModelLoader.h"
#include "model_data.h"
#include <iostream>
#include <string>

std::vector<char> EmbeddedModelLoader::loadCompressedModel() {
    extern unsigned char dx7_vae_model_pt_gz[];
    extern unsigned int dx7_vae_model_pt_gz_len;
    
    try {
        return decompressGzip(dx7_vae_model_pt_gz, dx7_vae_model_pt_gz_len);
    } catch (const std::exception& e) {
        std::cerr << "Failed to decompress embedded model: " << e.what() << std::endl;
        throw;
    }
}

std::vector<char> EmbeddedModelLoader::decompressGzip(const unsigned char* compressed_data, 
                                                     size_t compressed_size) {
    // Create memory input stream from compressed data
    juce::MemoryInputStream memoryStream(compressed_data, compressed_size, false);
    
    // Create GZIP decompressor
    juce::GZIPDecompressorInputStream gzipStream(&memoryStream, false);
    
    // Read all decompressed data
    std::vector<char> output;
    const int bufferSize = 4096;
    char buffer[bufferSize];
    
    while (!gzipStream.isExhausted()) {
        int bytesRead = gzipStream.read(buffer, bufferSize);
        if (bytesRead > 0) {
            output.insert(output.end(), buffer, buffer + bytesRead);
        } else {
            break;
        }
    }
    
    if (output.empty()) {
        throw std::runtime_error("Failed to decompress GZIP data");
    }
    
    return output;
}