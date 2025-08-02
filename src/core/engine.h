// src/core/engine.h
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>

// Forward declarations
class FFmpegEncoder;
class Slicer;
class ErasureCoder;
class Scheduler;
class PathMonitor;
class SenderReceiver;
class SmartCollector;

struct PathConfig {
    std::string ip;
    uint16_t port;
    
    PathConfig(const std::string& ip_addr, uint16_t port_num)
        : ip(ip_addr), port(port_num) {}
};

struct EngineConfig {
    int width;
    int height;
    int fps;
    int bitrate_kbps;
    size_t max_chunk_size;
    int k_chunks;
    int r_chunks;
    uint32_t jitter_buffer_ms;
    std::vector<PathConfig> paths;
    
    EngineConfig() : width(1280), height(720), fps(30), bitrate_kbps(3000),
                     max_chunk_size(1000), k_chunks(8), r_chunks(2), jitter_buffer_ms(100) {}
};

class Engine {
public:
    explicit Engine(const EngineConfig& config);
    ~Engine();
    
    // Start/stop engine
    void start();
    void stop();
    
    // Get configuration
    const EngineConfig& get_config() const;
    bool is_running() const;

private:
    EngineConfig config_;
    std::atomic<bool> running_{false};
    
    // Components
    std::unique_ptr<FFmpegEncoder> encoder_;
    std::unique_ptr<Slicer> slicer_;
    std::unique_ptr<ErasureCoder> erasure_coder_;
    std::unique_ptr<Scheduler> scheduler_;
    std::vector<std::unique_ptr<PathMonitor>> path_monitors_;
    std::vector<std::unique_ptr<SenderReceiver>> sender_receivers_;
    std::unique_ptr<SmartCollector> collector_;
    
    // Threads
    std::thread video_thread_;
    std::thread network_thread_;
    
    // Internal methods
    void initialize_components();
    void video_processing_loop();
    void network_processing_loop();
    void send_chunks(const std::vector<std::vector<uint8_t>>& data_chunks,
                     const std::vector<std::vector<uint8_t>>& fec_chunks,
                     uint32_t sequence_number);
    void process_complete_frame(const std::vector<uint8_t>& frame_data);
};