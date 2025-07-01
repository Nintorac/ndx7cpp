#include "EmbeddedModelLoader.h"
#include "model_data.h"
#include <iostream>
#include <string>
#include <juce_core/juce_core.h>

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
    juce::MemoryInputStream compressedStream(compressed_data, compressed_size, false);
    
    // Create GZIP decompressor with explicit GZIP format
    juce::GZIPDecompressorInputStream gzipStream(&compressedStream, false, 
                                                 juce::GZIPDecompressorInputStream::gzipFormat);
    
    // Read into a memory block
    juce::MemoryBlock decompressedBlock;
    auto totalBytesRead = gzipStream.readIntoMemoryBlock(decompressedBlock);
    
    if (totalBytesRead > 0) {
        // Convert to std::vector<char>
        const char* data = static_cast<const char*>(decompressedBlock.getData());
        std::vector<char> result(data, data + totalBytesRead);
        return result;
    }
    
    throw std::runtime_error("Failed to decompress GZIP data");
}