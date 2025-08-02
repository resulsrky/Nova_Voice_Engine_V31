#include "scheduler.h"
#include <algorithm>
#include <numeric>
#include <random>
#include <limits>

Scheduler::Scheduler() 
    : current_strategy_(ADAPTIVE), round_robin_index_(0) {
}

Scheduler::~Scheduler() = default;

void Scheduler::add_path(const std::string& ip, uint16_t port) {
    std::lock_guard<std::mutex> lock(paths_mutex_);
    
    // Check if path already exists
    auto it = std::find_if(paths_.begin(), paths_.end(),
                           [&](const PathInfo& path) {
                               return path.ip == ip && path.port == port;
                           });
    
    if (it == paths_.end()) {
        paths_.emplace_back(ip, port);
    }
}

void Scheduler::remove_path(const std::string& ip, uint16_t port) {
    std::lock_guard<std::mutex> lock(paths_mutex_);
    
    paths_.erase(
        std::remove_if(paths_.begin(), paths_.end(),
                       [&](const PathInfo& path) {
                           return path.ip == ip && path.port == port;
                       }),
        paths_.end()
    );
}

void Scheduler::update_path_metrics(const std::string& ip, uint16_t port,
                                  double rtt_ms, double loss_rate, double bandwidth_mbps) {
    std::lock_guard<std::mutex> lock(paths_mutex_);
    
    auto it = std::find_if(paths_.begin(), paths_.end(),
                           [&](const PathInfo& path) {
                               return path.ip == ip && path.port == port;
                           });
    
    if (it != paths_.end()) {
        it->rtt_ms = rtt_ms;
        it->loss_rate = loss_rate;
        it->bandwidth_mbps = bandwidth_mbps;
    }
}

PathInfo* Scheduler::get_next_path(Strategy strategy) {
    std::lock_guard<std::mutex> lock(paths_mutex_);
    
    if (paths_.empty()) {
        return nullptr;
    }
    
    // Filter active paths
    std::vector<PathInfo*> active_paths;
    for (auto& path : paths_) {
        if (path.is_active) {
            active_paths.push_back(&path);
        }
    }
    
    if (active_paths.empty()) {
        return nullptr;
    }
    
    switch (strategy) {
        case ROUND_ROBIN:
            return round_robin_select();
        case WEIGHTED_ROUND_ROBIN:
            return weighted_round_robin_select();
        case LOWEST_RTT:
            return lowest_rtt_select();
        case LOWEST_LOSS:
            return lowest_loss_select();
        case ADAPTIVE:
        default:
            return adaptive_select();
    }
}

PathInfo* Scheduler::round_robin_select() {
    if (paths_.empty()) return nullptr;
    
    PathInfo* selected = &paths_[round_robin_index_ % paths_.size()];
    round_robin_index_++;
    
    // Skip inactive paths
    while (!selected->is_active && round_robin_index_ < paths_.size() * 2) {
        selected = &paths_[round_robin_index_ % paths_.size()];
        round_robin_index_++;
    }
    
    return selected->is_active ? selected : nullptr;
}

PathInfo* Scheduler::weighted_round_robin_select() {
    if (paths_.empty()) return nullptr;
    
    // Calculate weights for all active paths
    std::vector<double> weights;
    std::vector<PathInfo*> active_paths;
    
    for (auto& path : paths_) {
        if (path.is_active) {
            weights.push_back(calculate_path_weight(path));
            active_paths.push_back(&path);
        }
    }
    
    if (active_paths.empty()) return nullptr;
    
    normalize_weights(weights);
    
    // Weighted random selection
    std::random_device rd;
    std::mt19937 gen(rd());
    std::discrete_distribution<> dist(weights.begin(), weights.end());
    
    return active_paths[dist(gen)];
}

PathInfo* Scheduler::lowest_rtt_select() {
    if (paths_.empty()) return nullptr;
    
    auto it = std::min_element(paths_.begin(), paths_.end(),
                               [](const PathInfo& a, const PathInfo& b) {
                                   return a.is_active && (!b.is_active || a.rtt_ms < b.rtt_ms);
                               });
    
    return (it != paths_.end() && it->is_active) ? &(*it) : nullptr;
}

PathInfo* Scheduler::lowest_loss_select() {
    if (paths_.empty()) return nullptr;
    
    auto it = std::min_element(paths_.begin(), paths_.end(),
                               [](const PathInfo& a, const PathInfo& b) {
                                   return a.is_active && (!b.is_active || a.loss_rate < b.loss_rate);
                               });
    
    return (it != paths_.end() && it->is_active) ? &(*it) : nullptr;
}

PathInfo* Scheduler::adaptive_select() {
    if (paths_.empty()) return nullptr;
    
    // Adaptive strategy: combine RTT and loss rate
    // Prefer paths with low RTT and low loss rate
    auto it = std::min_element(paths_.begin(), paths_.end(),
                               [](const PathInfo& a, const PathInfo& b) {
                                   if (!a.is_active && !b.is_active) return false;
                                   if (!a.is_active) return false;
                                   if (!b.is_active) return true;
                                   
                                   // Score = RTT * (1 + loss_rate * 10)
                                   double score_a = a.rtt_ms * (1.0 + a.loss_rate * 10.0);
                                   double score_b = b.rtt_ms * (1.0 + b.loss_rate * 10.0);
                                   return score_a < score_b;
                               });
    
    return (it != paths_.end() && it->is_active) ? &(*it) : nullptr;
}

double Scheduler::calculate_path_weight(const PathInfo& path) const {
    if (!path.is_active) return 0.0;
    
    // Weight based on RTT and loss rate
    // Lower RTT and loss rate = higher weight
    double rtt_weight = 1.0 / (path.rtt_ms + 1.0); // Avoid division by zero
    double loss_weight = 1.0 - path.loss_rate;
    double bandwidth_weight = path.bandwidth_mbps / 100.0; // Normalize to 0-1
    
    return rtt_weight * loss_weight * (1.0 + bandwidth_weight);
}

void Scheduler::normalize_weights(std::vector<double>& weights) const {
    if (weights.empty()) return;
    
    double sum = std::accumulate(weights.begin(), weights.end(), 0.0);
    if (sum > 0.0) {
        for (auto& weight : weights) {
            weight /= sum;
        }
    } else {
        // If all weights are zero, make them equal
        double equal_weight = 1.0 / weights.size();
        for (auto& weight : weights) {
            weight = equal_weight;
        }
    }
}

std::vector<PathInfo> Scheduler::get_paths() const {
    std::lock_guard<std::mutex> lock(paths_mutex_);
    return paths_;
}

bool Scheduler::has_active_paths() const {
    std::lock_guard<std::mutex> lock(paths_mutex_);
    return std::any_of(paths_.begin(), paths_.end(),
                       [](const PathInfo& path) { return path.is_active; });
}

size_t Scheduler::get_path_count() const {
    std::lock_guard<std::mutex> lock(paths_mutex_);
    return paths_.size();
} 