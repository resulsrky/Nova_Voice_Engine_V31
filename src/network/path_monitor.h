// src/network/path_monitor.h
#pragma once
#include <string>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

struct PathMetrics {
    double rtt_ms;
    double loss_rate;
    double bandwidth_mbps;
    uint64_t packets_sent;
    uint64_t packets_received;
    uint64_t packets_lost;
    
    PathMetrics() : rtt_ms(0.0), loss_rate(0.0), bandwidth_mbps(0.0),
                    packets_sent(0), packets_received(0), packets_lost(0) {}
};

class PathMonitor {
public:
    using MetricsCallback = std::function<void(const std::string&, uint16_t, const PathMetrics&)>;
    
    PathMonitor(const std::string& ip, uint16_t port);
    ~PathMonitor();
    
    // Start monitoring
    void start();
    
    // Stop monitoring
    void stop();
    
    // Set callback for metrics updates
    void set_metrics_callback(MetricsCallback callback);
    
    // Update metrics manually
    void update_rtt(double rtt_ms);
    void update_loss_rate(double loss_rate);
    void update_bandwidth(double bandwidth_mbps);
    void increment_packets_sent();
    void increment_packets_received();
    void increment_packets_lost();
    
    // Get current metrics
    PathMetrics get_metrics() const;
    
    // Get path info
    std::string get_ip() const { return ip_; }
    uint16_t get_port() const { return port_; }
    
    // Check if monitoring is active
    bool is_active() const { return running_.load(); }

private:
    std::string ip_;
    uint16_t port_;
    std::atomic<bool> running_{false};
    std::thread monitor_thread_;
    mutable std::mutex metrics_mutex_;
    PathMetrics metrics_;
    MetricsCallback metrics_callback_;
    
    // Monitoring parameters
    std::chrono::milliseconds update_interval_{1000}; // 1 second
    std::chrono::milliseconds rtt_window_{5000}; // 5 seconds for RTT calculation
    
    // Monitoring loop
    void monitor_loop();
    
    // Calculate metrics
    void calculate_metrics();
    
    // Notify callback
    void notify_metrics_update();
};