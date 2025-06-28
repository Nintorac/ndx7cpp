#pragma once

#include <vector>
#include <memory>
#include <stdexcept>
#include <juce_core/juce_core.h>

class EmbeddedModelLoader {
public:
    static std::vector<char> loadCompressedModel();
    
private:
    static std::vector<char> decompressGzip(const unsigned char* compressed_data, 
                                           size_t compressed_size);
};