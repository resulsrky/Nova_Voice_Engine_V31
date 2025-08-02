// src/transport/chunk.h
#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <memory>

struct Chunk {
    uint32_t sequence_number;
    uint32_t timestamp;
    uint16_t chunk_id;
    uint16_t total_chunks;
    uint16_t data_size;
    std::vector<uint8_t> data;
    bool is_fec;
    uint16_t fec_group_id;
    
    Chunk() : sequence_number(0), timestamp(0), chunk_id(0), 
               total_chunks(0), data_size(0), is_fec(false), fec_group_id(0) {}
    
    Chunk(uint32_t seq, uint32_t ts, uint16_t id, uint16_t total, 
          const std::vector<uint8_t>& d, bool fec = false, uint16_t fec_id = 0)
        : sequence_number(seq), timestamp(ts), chunk_id(id), 
          total_chunks(total), data_size(d.size()), data(d), 
          is_fec(fec), fec_group_id(fec_id) {}
    
    // Serialize chunk to byte array
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> result;
        result.reserve(20 + data.size()); // Header + data
        
        // Header: 20 bytes
        result.insert(result.end(), (uint8_t*)&sequence_number, (uint8_t*)&sequence_number + 4);
        result.insert(result.end(), (uint8_t*)&timestamp, (uint8_t*)&timestamp + 4);
        result.insert(result.end(), (uint8_t*)&chunk_id, (uint8_t*)&chunk_id + 2);
        result.insert(result.end(), (uint8_t*)&total_chunks, (uint8_t*)&total_chunks + 2);
        result.insert(result.end(), (uint8_t*)&data_size, (uint8_t*)&data_size + 2);
        result.insert(result.end(), (uint8_t*)&is_fec, (uint8_t*)&is_fec + 1);
        result.insert(result.end(), (uint8_t*)&fec_group_id, (uint8_t*)&fec_group_id + 2);
        
        // Padding to align to 20 bytes
        result.insert(result.end(), 3, 0);
        
        // Data
        result.insert(result.end(), data.begin(), data.end());
        
        return result;
    }
    
    // Deserialize chunk from byte array
    static Chunk deserialize(const std::vector<uint8_t>& buffer) {
        if (buffer.size() < 20) {
            throw std::runtime_error("Buffer too small for chunk header");
        }
        
        Chunk chunk;
        size_t offset = 0;
        
        // Read header
        std::memcpy(&chunk.sequence_number, buffer.data() + offset, 4);
        offset += 4;
        std::memcpy(&chunk.timestamp, buffer.data() + offset, 4);
        offset += 4;
        std::memcpy(&chunk.chunk_id, buffer.data() + offset, 2);
        offset += 2;
        std::memcpy(&chunk.total_chunks, buffer.data() + offset, 2);
        offset += 2;
        std::memcpy(&chunk.data_size, buffer.data() + offset, 2);
        offset += 2;
        std::memcpy(&chunk.is_fec, buffer.data() + offset, 1);
        offset += 1;
        std::memcpy(&chunk.fec_group_id, buffer.data() + offset, 2);
        offset += 2;
        offset += 3; // Skip padding
        
        // Read data
        if (offset + chunk.data_size <= buffer.size()) {
            chunk.data.assign(buffer.begin() + offset, buffer.begin() + offset + chunk.data_size);
        }
        
        return chunk;
    }
};

using ChunkPtr = std::shared_ptr<Chunk>;