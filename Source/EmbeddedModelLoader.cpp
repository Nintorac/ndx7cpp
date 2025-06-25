#include "EmbeddedModelLoader.h"
#include "model_data.h"
#include <iostream>

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
    // Start with a reasonable buffer size
    size_t buffer_size = compressed_size * 4;
    std::vector<char> output(buffer_size);
    
    z_stream stream = {};
    stream.next_in = const_cast<Bytef*>(compressed_data);
    stream.avail_in = static_cast<uInt>(compressed_size);
    stream.next_out = reinterpret_cast<Bytef*>(output.data());
    stream.avail_out = static_cast<uInt>(output.size());
    
    // Initialize for gzip decompression (windowBits = 15 + 16 for gzip)
    int result = inflateInit2(&stream, 15 + 16);
    if (result != Z_OK) {
        throw std::runtime_error("Failed to initialize zlib: " + std::to_string(result));
    }
    
    // Decompress
    result = inflate(&stream, Z_FINISH);
    
    if (result == Z_BUF_ERROR && stream.avail_out == 0) {
        // Need more output space, try with larger buffer
        inflateEnd(&stream);
        
        buffer_size *= 2;
        output.resize(buffer_size);
        
        // Restart decompression
        stream = {};
        stream.next_in = const_cast<Bytef*>(compressed_data);
        stream.avail_in = static_cast<uInt>(compressed_size);
        stream.next_out = reinterpret_cast<Bytef*>(output.data());
        stream.avail_out = static_cast<uInt>(output.size());
        
        result = inflateInit2(&stream, 15 + 16);
        if (result != Z_OK) {
            throw std::runtime_error("Failed to reinitialize zlib: " + std::to_string(result));
        }
        
        result = inflate(&stream, Z_FINISH);
    }
    
    if (result != Z_STREAM_END) {
        inflateEnd(&stream);
        throw std::runtime_error("Decompression failed: " + std::to_string(result));
    }
    
    // Resize to actual decompressed size
    size_t actual_size = output.size() - stream.avail_out;
    output.resize(actual_size);
    
    inflateEnd(&stream);
    return output;
}