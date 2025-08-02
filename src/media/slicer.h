// slicer.h
#pragma once
#include <vector>
#include <cstdint>
#include <memory>

class Slicer {
public:
    explicit Slicer(size_t max_chunk_size);
    
    // Slice data into chunks
    std::vector<std::vector<uint8_t>> slice(const std::vector<uint8_t>& data);
    
    // Unslice chunks back to data
    std::vector<uint8_t> unslice(const std::vector<std::vector<uint8_t>>& chunks);
    
    // Slice data with header information
    std::vector<std::vector<uint8_t>> slice_with_header(const std::vector<uint8_t>& data, 
                                                       uint32_t sequence_number);
    
    // Unslice chunks with header
    std::vector<uint8_t> unslice_with_header(const std::vector<std::vector<uint8_t>>& chunks);
    
    // Get/set max chunk size
    size_t get_max_chunk_size() const;
    void set_max_chunk_size(size_t size);

private:
    size_t max_chunk_size_;
};