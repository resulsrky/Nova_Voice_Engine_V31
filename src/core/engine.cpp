// src/core/engine.cpp
#include "engine.h"
#include "../media/ffmpeg_encoder.h"
#include "../media/slicer.h"
#include "../media/erasure_coder.h"
#include "../network/sender_receiver.h"
#include "../network/scheduler.h"
#include "../network/path_monitor.h"
#include "../transport/smart_collector.h"
#include "../common/logger.h"
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>
#include <memory>

Engine::Engine(const EngineConfig& config) 
    : config_(config), running_(false) {
    
    LOG_INFO("Nova Engine V3 başlatılıyor...");
    
    // Initialize components
    initialize_components();
}

Engine::~Engine() {
    stop();
}

void Engine::initialize_components() {
    try {
        // Initialize FFmpeg encoder (if available)
        FFmpegEncoder::EncoderConfig encoder_config(
            config_.width, config_.height, config_.fps, 
            config_.bitrate_kbps, "libx264", "veryfast", "grain"
        );
        encoder_ = std::make_unique<FFmpegEncoder>(encoder_config);
        
        // Try to initialize encoder, but don't fail if FFmpeg is not available
        if (!encoder_->initialize()) {
            LOG_WARNING("FFmpeg encoder başlatılamadı. Video encoding devre dışı.");
        }
        
        // Initialize slicer
        slicer_ = std::make_unique<Slicer>(config_.max_chunk_size);
        
        // Initialize erasure coder
        ErasureCoder::CodingParams coding_params(config_.k_chunks, config_.r_chunks);
        erasure_coder_ = std::make_unique<ErasureCoder>(coding_params);
        
        // Initialize scheduler
        scheduler_ = std::make_unique<Scheduler>();
        
        // Initialize path monitors
        for (const auto& path : config_.paths) {
            auto monitor = std::make_unique<PathMonitor>(path.ip, path.port);
            monitor->set_metrics_callback([this](const std::string& ip, uint16_t port, const PathMetrics& metrics) {
                scheduler_->update_path_metrics(ip, port, metrics.rtt_ms, metrics.loss_rate, metrics.bandwidth_mbps);
            });
            path_monitors_.push_back(std::move(monitor));
        }
        
        // Initialize sender/receivers
        for (const auto& path : config_.paths) {
            auto sender = std::make_unique<SenderReceiver>(path.ip, path.port);
            if (!sender->initialize()) {
                throw std::runtime_error("Sender/Receiver başlatılamadı: " + path.ip + ":" + std::to_string(path.port));
            }
            sender_receivers_.push_back(std::move(sender));
        }
        
        // Initialize smart collector
        collector_ = std::make_unique<SmartCollector>(config_.jitter_buffer_ms);
        
        LOG_INFO("Tüm bileşenler başarıyla başlatıldı");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Bileşen başlatma hatası: " + std::string(e.what()));
        throw;
    }
}

void Engine::start() {
    if (running_.load()) {
        LOG_WARNING("Engine zaten çalışıyor");
        return;
    }
    
    LOG_INFO("Engine başlatılıyor...");
    running_ = true;
    
    // Start path monitors
    for (auto& monitor : path_monitors_) {
        monitor->start();
    }
    
    // Start sender/receiver threads
    for (auto& sender : sender_receivers_) {
        sender->start();
    }
    
    // Start collector
    collector_->start();
    
    // Start main processing threads
    video_thread_ = std::thread(&Engine::video_processing_loop, this);
    network_thread_ = std::thread(&Engine::network_processing_loop, this);
    
    LOG_INFO("Engine başarıyla başlatıldı");
}

void Engine::stop() {
    if (!running_.load()) {
        return;
    }
    
    LOG_INFO("Engine durduruluyor...");
    running_ = false;
    
    // Stop threads
    if (video_thread_.joinable()) {
        video_thread_.join();
    }
    if (network_thread_.joinable()) {
        network_thread_.join();
    }
    
    // Stop components
    for (auto& monitor : path_monitors_) {
        monitor->stop();
    }
    
    for (auto& sender : sender_receivers_) {
        sender->stop();
    }
    
    if (collector_) {
        collector_->stop();
    }
    
    LOG_INFO("Engine durduruldu");
}

void Engine::video_processing_loop() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        LOG_ERROR("Kamera açılamadı");
        return;
    }
    
    cap.set(cv::CAP_PROP_FRAME_WIDTH, config_.width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, config_.height);
    cap.set(cv::CAP_PROP_FPS, config_.fps);
    
    cv::Mat frame;
    uint32_t frame_sequence = 0;
    
    while (running_.load()) {
        cap >> frame;
        if (frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        try {
            // Convert frame to RGB
            cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
            
            // Encode frame (if encoder is available)
            std::vector<uint8_t> encoded_data;
            if (encoder_ && encoder_->is_initialized()) {
                encoded_data = encoder_->encode_frame(
                    frame.data, frame.cols, frame.rows
                );
            } else {
                // Fallback: convert to JPEG
                std::vector<uchar> buffer;
                std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 80};
                cv::imencode(".jpg", frame, buffer, params);
                encoded_data.assign(buffer.begin(), buffer.end());
            }
            
            if (!encoded_data.empty()) {
                // Slice encoded data
                auto chunks = slicer_->slice_with_header(encoded_data, frame_sequence);
                
                // Add FEC chunks
                auto fec_chunks = erasure_coder_->encode(encoded_data);
                
                // Send chunks through network
                send_chunks(chunks, fec_chunks, frame_sequence);
                
                frame_sequence++;
            }
            
        } catch (const std::exception& e) {
            LOG_ERROR("Video işleme hatası: " + std::string(e.what()));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / config_.fps));
    }
    
    cap.release();
}

void Engine::network_processing_loop() {
    while (running_.load()) {
        try {
            // Process received chunks
            for (auto& sender : sender_receivers_) {
                auto received_chunks = sender->receive_chunks();
                
                for (const auto& chunk_data : received_chunks) {
                    // Parse chunk header
                    if (chunk_data.size() >= 12) {
                        uint32_t sequence_number;
                        uint16_t chunk_id, total_chunks, chunk_size;
                        
                        std::memcpy(&sequence_number, chunk_data.data(), 4);
                        std::memcpy(&chunk_id, chunk_data.data() + 4, 2);
                        std::memcpy(&total_chunks, chunk_data.data() + 6, 2);
                        std::memcpy(&chunk_size, chunk_data.data() + 8, 2);
                        
                        // Add to collector
                        collector_->add_chunk(sequence_number, chunk_id, total_chunks, 
                                            std::vector<uint8_t>(chunk_data.begin() + 12, chunk_data.end()));
                    }
                }
            }
            
            // Process complete frames from collector
            auto complete_frames = collector_->get_complete_frames();
            for (const auto& frame_data : complete_frames) {
                process_complete_frame(frame_data);
            }
            
        } catch (const std::exception& e) {
            LOG_ERROR("Network işleme hatası: " + std::string(e.what()));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void Engine::send_chunks(const std::vector<std::vector<uint8_t>>& data_chunks,
                        const std::vector<std::vector<uint8_t>>& fec_chunks,
                        uint32_t sequence_number) {
    
    // Get best path from scheduler
    auto path = scheduler_->get_next_path();
    if (!path) {
        LOG_WARNING("Aktif path bulunamadı");
        return;
    }
    
    // Find corresponding sender
    for (auto& sender : sender_receivers_) {
        if (sender->get_remote_ip() == path->ip && sender->get_remote_port() == path->port) {
            // Send data chunks
            for (const auto& chunk : data_chunks) {
                sender->send_chunk(chunk);
            }
            
            // Send FEC chunks
            for (const auto& chunk : fec_chunks) {
                sender->send_chunk(chunk);
            }
            
            break;
        }
    }
}

void Engine::process_complete_frame(const std::vector<uint8_t>& frame_data) {
    try {
        // Decode frame
        std::vector<uchar> frame_buffer(frame_data.begin(), frame_data.end());
        cv::Mat frame = cv::imdecode(frame_buffer, cv::IMREAD_COLOR);
        
        if (!frame.empty()) {
            // Display frame
            cv::imshow("Received Frame", frame);
            cv::waitKey(1);
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Frame işleme hatası: " + std::string(e.what()));
    }
}

const EngineConfig& Engine::get_config() const {
    return config_;
}

bool Engine::is_running() const {
    return running_.load();
}