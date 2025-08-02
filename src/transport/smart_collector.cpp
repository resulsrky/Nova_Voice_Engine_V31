// smart_collector.cpp
#include "smart_collector.h"
#include "../common/logger.h"
#include <algorithm>
#include <chrono>
#include <stdexcept>

SmartCollector::SmartCollector(uint32_t jitter_buffer_ms)
    : jitter_buffer_ms_(jitter_buffer_ms), running_(false) {
    
    if (jitter_buffer_ms == 0) {
        throw std::invalid_argument("Jitter buffer süresi 0 olamaz");
    }
}

SmartCollector::~SmartCollector() {
    stop();
}

void SmartCollector::start() {
    if (running_.load()) {
        LOG_WARNING("SmartCollector zaten çalışıyor");
        return;
    }
    
    running_ = true;
    collector_thread_ = std::thread(&SmartCollector::collector_loop, this);
    
    LOG_INFO("SmartCollector başlatıldı (jitter buffer: " + std::to_string(jitter_buffer_ms_) + "ms)");
}

void SmartCollector::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_ = false;
    
    if (collector_thread_.joinable()) {
        collector_thread_.join();
    }
    
    LOG_INFO("SmartCollector durduruldu");
}

void SmartCollector::add_chunk(uint32_t sequence_number, uint16_t chunk_id, 
                              uint16_t total_chunks, const std::vector<uint8_t>& chunk_data) {
    if (!running_.load()) {
        return;
    }
    
    try {
        std::lock_guard<std::mutex> lock(chunks_mutex_);
        
        // Find or create frame buffer for this sequence
        auto it = frame_buffers_.find(sequence_number);
        if (it == frame_buffers_.end()) {
            // Create new frame buffer
            frame_buffers_[sequence_number] = std::make_unique<FrameBuffer>(total_chunks);
        }
        
        FrameBuffer& frame_buffer = *it->second;
        
        // Add chunk to frame buffer
        if (chunk_id < total_chunks) {
            frame_buffer.chunks[chunk_id] = chunk_data;
            frame_buffer.received_chunks++;
            
            // Check if frame is complete
            if (frame_buffer.received_chunks == total_chunks) {
                complete_frames_.push_back(sequence_number);
            }
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Chunk ekleme hatası: " + std::string(e.what()));
    }
}

std::vector<std::vector<uint8_t>> SmartCollector::get_complete_frames() {
    std::vector<std::vector<uint8_t>> frames;
    
    try {
        std::lock_guard<std::mutex> lock(chunks_mutex_);
        
        // Process complete frames
        for (uint32_t sequence_number : complete_frames_) {
            auto it = frame_buffers_.find(sequence_number);
            if (it != frame_buffers_.end()) {
                FrameBuffer& frame_buffer = *it->second;
                
                // Combine all chunks
                std::vector<uint8_t> frame_data;
                for (const auto& chunk : frame_buffer.chunks) {
                    if (!chunk.empty()) {
                        frame_data.insert(frame_data.end(), chunk.begin(), chunk.end());
                    }
                }
                
                if (!frame_data.empty()) {
                    frames.push_back(std::move(frame_data));
                }
                
                // Remove processed frame
                frame_buffers_.erase(it);
            }
        }
        
        complete_frames_.clear();
        
    } catch (const std::exception& e) {
        LOG_ERROR("Frame alma hatası: " + std::string(e.what()));
    }
    
    return frames;
}

void SmartCollector::collector_loop() {
    auto last_cleanup = std::chrono::steady_clock::now();
    std::chrono::milliseconds cleanup_interval(1000); // Cleanup every second
    
    while (running_.load()) {
        try {
            auto now = std::chrono::steady_clock::now();
            
            // Periodic cleanup of old frames
            if (now - last_cleanup >= cleanup_interval) {
                cleanup_old_frames();
                last_cleanup = now;
            }
            
        } catch (const std::exception& e) {
            LOG_ERROR("Collector döngüsü hatası: " + std::string(e.what()));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void SmartCollector::cleanup_old_frames() {
    std::lock_guard<std::mutex> lock(chunks_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto cutoff_time = now - std::chrono::milliseconds(jitter_buffer_ms_);
    
    // Remove old frame buffers
    auto it = frame_buffers_.begin();
    while (it != frame_buffers_.end()) {
        if (it->second->timestamp < cutoff_time) {
            it = frame_buffers_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Remove old complete frames
    auto frame_it = complete_frames_.begin();
    while (frame_it != complete_frames_.end()) {
        auto buffer_it = frame_buffers_.find(*frame_it);
        if (buffer_it == frame_buffers_.end() || 
            buffer_it->second->timestamp < cutoff_time) {
            frame_it = complete_frames_.erase(frame_it);
        } else {
            ++frame_it;
        }
    }
}

size_t SmartCollector::get_frame_count() const {
    std::lock_guard<std::mutex> lock(chunks_mutex_);
    return frame_buffers_.size();
}

size_t SmartCollector::get_complete_frame_count() const {
    std::lock_guard<std::mutex> lock(chunks_mutex_);
    return complete_frames_.size();
}

uint32_t SmartCollector::get_jitter_buffer_ms() const {
    return jitter_buffer_ms_;
}

bool SmartCollector::is_running() const {
    return running_.load();
}

// FrameBuffer constructor
SmartCollector::FrameBuffer::FrameBuffer(uint16_t total_chunks)
    : chunks(total_chunks), received_chunks(0), 
      timestamp(std::chrono::steady_clock::now()) {
}