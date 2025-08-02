// scheduler.h
#pragma once
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

class PathMonitor;

struct PathInfo {
    std::string ip;
    uint16_t port;
    double rtt_ms;
    double loss_rate;
    double bandwidth_mbps;
    bool is_active;
    
    PathInfo(const std::string& ip_addr, uint16_t port_num)
        : ip(ip_addr), port(port_num), rtt_ms(0.0), 
          loss_rate(0.0), bandwidth_mbps(0.0), is_active(true) {}
};

class Scheduler {
public:
    enum Strategy {
        ROUND_ROBIN,
        WEIGHTED_ROUND_ROBIN,
        LOWEST_RTT,
        LOWEST_LOSS,
        ADAPTIVE
    };
    
    Scheduler();
    ~Scheduler();
    
    // Add a path to the scheduler
    void add_path(const std::string& ip, uint16_t port);
    
    // Remove a path from the scheduler
    void remove_path(const std::string& ip, uint16_t port);
    
    // Update path metrics
    void update_path_metrics(const std::string& ip, uint16_t port, 
                           double rtt_ms, double loss_rate, double bandwidth_mbps);
    
    // Get next path based on strategy
    PathInfo* get_next_path(Strategy strategy = ADAPTIVE);
    
    // Set scheduling strategy
    void set_strategy(Strategy strategy) { current_strategy_ = strategy; }
    
    // Get current strategy
    Strategy get_strategy() const { return current_strategy_; }
    
    // Get all paths
    std::vector<PathInfo> get_paths() const;
    
    // Check if any paths are available
    bool has_active_paths() const;
    
    // Get path count
    size_t get_path_count() const;

private:
    std::vector<PathInfo> paths_;
    Strategy current_strategy_;
    size_t round_robin_index_;
    mutable std::mutex paths_mutex_;
    
    // Strategy implementations
    PathInfo* round_robin_select();
    PathInfo* weighted_round_robin_select();
    PathInfo* lowest_rtt_select();
    PathInfo* lowest_loss_select();
    PathInfo* adaptive_select();
    
    // Helper functions
    double calculate_path_weight(const PathInfo& path) const;
    void normalize_weights(std::vector<double>& weights) const;
};