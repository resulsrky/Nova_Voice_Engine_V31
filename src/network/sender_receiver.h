// sender_receiver.h
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <atomic>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>

class SenderReceiver {
public:
    SenderReceiver(const std::string& remote_ip, uint16_t remote_port);
    ~SenderReceiver();
    
    // Initialize socket
    bool initialize();
    
    // Start/stop receiver thread
    void start();
    void stop();
    
    // Send chunk data
    void send_chunk(const std::vector<uint8_t>& chunk_data);
    
    // Receive chunks (non-blocking)
    std::vector<std::vector<uint8_t>> receive_chunks();
    
    // Get received chunks from internal buffer
    std::vector<std::vector<uint8_t>> get_received_chunks();
    
    // Get connection info
    std::string get_remote_ip() const;
    uint16_t get_remote_port() const;
    uint16_t get_local_port() const;
    bool is_running() const;

private:
    std::string remote_ip_;
    uint16_t remote_port_;
    uint16_t local_port_;
    int sockfd_;
    struct sockaddr_in remote_addr_;
    
    std::atomic<bool> running_{false};
    std::thread receive_thread_;
    std::mutex received_chunks_mutex_;
    std::vector<std::vector<uint8_t>> received_chunks_;
    
    // Internal methods
    void receive_loop();
};