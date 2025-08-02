#include "slicer.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

Slicer::Slicer(size_t max_chunk_size) : max_chunk_size_(max_chunk_size) {
    if (max_chunk_size == 0) {
        throw std::invalid_argument("Max chunk size must be greater than 0");
    }
}

std::vector<std::vector<uint8_t>> Slicer::slice(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return {};
    }
    
    std::vector<std::vector<uint8_t>> chunks;
    size_t offset = 0;
    
    while (offset < data.size()) {
        size_t chunk_size = std::min(max_chunk_size_, data.size() - offset);
        
        std::vector<uint8_t> chunk(data.begin() + offset, 
                                   data.begin() + offset + chunk_size);
        chunks.push_back(std::move(chunk));
        
        offset += chunk_size;
    }
    
    return chunks;
}

std::vector<uint8_t> Slicer::unslice(const std::vector<std::vector<uint8_t>>& chunks) {
    if (chunks.empty()) {
        return {};
    }
    
    // Calculate total size
    size_t total_size = 0;
    for (const auto& chunk : chunks) {
        total_size += chunk.size();
    }
    
    std::vector<uint8_t> result;
    result.reserve(total_size);
    
    // Combine all chunks
    for (const auto& chunk : chunks) {
        result.insert(result.end(), chunk.begin(), chunk.end());
    }
    
    return result;
}

std::vector<std::vector<uint8_t>> Slicer::slice_with_header(const std::vector<uint8_t>& data, 
                                                           uint32_t sequence_number) {
    if (data.empty()) {
        return {};
    }
    
    std::vector<std::vector<uint8_t>> chunks;
    size_t offset = 0;
    uint16_t chunk_id = 0;
    
    while (offset < data.size()) {
        size_t chunk_size = std::min(max_chunk_size_, data.size() - offset);
        
        // Create chunk with header
        std::vector<uint8_t> chunk;
        chunk.reserve(12 + chunk_size); // Header + data
        
        // Header: 12 bytes
        // - sequence_number (4 bytes)
        // - chunk_id (2 bytes)
        // - total_chunks (2 bytes)
        // - chunk_size (2 bytes)
        // - reserved (2 bytes)
        
        uint16_t total_chunks = (data.size() + max_chunk_size_ - 1) / max_chunk_size_;
        
        chunk.insert(chunk.end(), (uint8_t*)&sequence_number, (uint8_t*)&sequence_number + 4);
        chunk.insert(chunk.end(), (uint8_t*)&chunk_id, (uint8_t*)&chunk_id + 2);
        chunk.insert(chunk.end(), (uint8_t*)&total_chunks, (uint8_t*)&total_chunks + 2);
        chunk.insert(chunk.end(), (uint8_t*)&chunk_size, (uint8_t*)&chunk_size + 2);
        chunk.insert(chunk.end(), 2, 0); // Reserved bytes
        
        // Add data
        chunk.insert(chunk.end(), data.begin() + offset, data.begin() + offset + chunk_size);
        
        chunks.push_back(std::move(chunk));
        
        offset += chunk_size;
        chunk_id++;
    }
    
    return chunks;
}

std::vector<uint8_t> Slicer::unslice_with_header(const std::vector<std::vector<uint8_t>>& chunks) {
    if (chunks.empty()) {
        return {};
    }
    
    std::vector<uint8_t> result;
    
    for (const auto& chunk : chunks) {
        if (chunk.size() < 12) {
            throw std::runtime_error("Chunk too small for header");
        }
        
        // Skip header (12 bytes) and add data
        result.insert(result.end(), chunk.begin() + 12, chunk.end());
    }
    
    return result;
}

size_t Slicer::get_max_chunk_size() const {
    return max_chunk_size_;
}

void Slicer::set_max_chunk_size(size_t size) {
    if (size == 0) {
        throw std::invalid_argument("Max chunk size must be greater than 0");
    }
    max_chunk_size_ = size;
} 