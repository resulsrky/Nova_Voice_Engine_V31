// smart_collector.h
#pragma once
#include <vector>
#include <cstdint>
#include <memory>
#include <map>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>

class SmartCollector {
public:
    explicit SmartCollector(uint32_t jitter_buffer_ms);
    ~SmartCollector();
    
    // Start/stop collector
    void start();
    void stop();
    
    // Add chunk to collector
    void add_chunk(uint32_t sequence_number, uint16_t chunk_id, 
                   uint16_t total_chunks, const std::vector<uint8_t>& chunk_data);
    
    // Get complete frames
    std::vector<std::vector<uint8_t>> get_complete_frames();
    
    // Get statistics
    size_t get_frame_count() const;
    size_t get_complete_frame_count() const;
    uint32_t get_jitter_buffer_ms() const;
    bool is_running() const;

private:
    struct FrameBuffer {
        std::vector<std::vector<uint8_t>> chunks;
        uint16_t received_chunks;
        std::chrono::steady_clock::time_point timestamp;
        
        explicit FrameBuffer(uint16_t total_chunks);
    };
    
    uint32_t jitter_buffer_ms_;
    std::atomic<bool> running_{false};
    std::thread collector_thread_;
    mutable std::mutex chunks_mutex_;
    
    std::map<uint32_t, std::unique_ptr<FrameBuffer>> frame_buffers_;
    std::vector<uint32_t> complete_frames_;
    
    // Internal methods
    void collector_loop();
    void cleanup_old_frames();
};