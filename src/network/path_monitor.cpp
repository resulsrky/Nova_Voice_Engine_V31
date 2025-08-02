// src/network/path_monitor.cpp
#include "path_monitor.h"
#include "../common/logger.h"
#include <algorithm>
#include <chrono>
#include <cmath>

PathMonitor::PathMonitor(const std::string& ip, uint16_t port)
    : ip_(ip), port_(port) {
}

PathMonitor::~PathMonitor() {
    stop();
}

void PathMonitor::start() {
    if (running_.load()) {
        LOG_WARNING("PathMonitor zaten çalışıyor: " + ip_ + ":" + std::to_string(port_));
        return;
    }
    
    running_ = true;
    monitor_thread_ = std::thread(&PathMonitor::monitor_loop, this);
    
    LOG_INFO("PathMonitor başlatıldı: " + ip_ + ":" + std::to_string(port_));
}

void PathMonitor::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_ = false;
    
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    
    LOG_INFO("PathMonitor durduruldu: " + ip_ + ":" + std::to_string(port_));
}

void PathMonitor::set_metrics_callback(MetricsCallback callback) {
    metrics_callback_ = std::move(callback);
}

void PathMonitor::update_rtt(double rtt_ms) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_.rtt_ms = rtt_ms;
}

void PathMonitor::update_loss_rate(double loss_rate) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_.loss_rate = loss_rate;
}

void PathMonitor::update_bandwidth(double bandwidth_mbps) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_.bandwidth_mbps = bandwidth_mbps;
}

void PathMonitor::increment_packets_sent() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_.packets_sent++;
}

void PathMonitor::increment_packets_received() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_.packets_received++;
}

void PathMonitor::increment_packets_lost() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_.packets_lost++;
}

PathMetrics PathMonitor::get_metrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return metrics_;
}

void PathMonitor::monitor_loop() {
    auto last_update = std::chrono::steady_clock::now();
    
    while (running_.load()) {
        try {
            auto now = std::chrono::steady_clock::now();
            
            if (now - last_update >= update_interval_) {
                calculate_metrics();
                notify_metrics_update();
                last_update = now;
            }
            
        } catch (const std::exception& e) {
            LOG_ERROR("PathMonitor döngüsü hatası: " + std::string(e.what()));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void PathMonitor::calculate_metrics() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    // Calculate loss rate
    uint64_t total_packets = metrics_.packets_sent + metrics_.packets_received;
    if (total_packets > 0) {
        metrics_.loss_rate = static_cast<double>(metrics_.packets_lost) / total_packets;
    } else {
        metrics_.loss_rate = 0.0;
    }
    
    // Apply exponential moving average for RTT
    // This is a simplified calculation - in a real implementation,
    // you would maintain a window of RTT measurements
    if (metrics_.rtt_ms > 0) {
        // Simple smoothing: new_rtt = 0.9 * old_rtt + 0.1 * new_measurement
        // This is just an example - actual implementation would be more sophisticated
    }
    
    // Calculate bandwidth (simplified)
    // In a real implementation, you would measure bytes transferred over time
    if (metrics_.bandwidth_mbps <= 0) {
        // Estimate based on packet rate and size
        // This is a placeholder calculation
        metrics_.bandwidth_mbps = 10.0; // Default estimate
    }
}

void PathMonitor::notify_metrics_update() {
    if (metrics_callback_) {
        PathMetrics current_metrics;
        {
            std::lock_guard<std::mutex> lock(metrics_mutex_);
            current_metrics = metrics_;
        }
        
        try {
            metrics_callback_(ip_, port_, current_metrics);
        } catch (const std::exception& e) {
            LOG_ERROR("Metrics callback hatası: " + std::string(e.what()));
        }
    }
}