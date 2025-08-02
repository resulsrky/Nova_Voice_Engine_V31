// sender_receiver.cpp
#include "sender_receiver.h"
#include "../common/logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>

SenderReceiver::SenderReceiver(const std::string& remote_ip, uint16_t remote_port)
    : remote_ip_(remote_ip), remote_port_(remote_port), sockfd_(-1), running_(false) {
}

SenderReceiver::~SenderReceiver() {
    stop();
    if (sockfd_ >= 0) {
        close(sockfd_);
    }
}

bool SenderReceiver::initialize() {
    try {
        // Create UDP socket
        sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd_ < 0) {
            LOG_ERROR("Socket oluşturulamadı: " + std::string(strerror(errno)));
            return false;
        }
        
        // Set socket options
        int reuse = 1;
        if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
            LOG_ERROR("SO_REUSEADDR ayarlanamadı: " + std::string(strerror(errno)));
            return false;
        }
        
        // Set non-blocking mode
        int flags = fcntl(sockfd_, F_GETFL, 0);
        if (flags < 0) {
            LOG_ERROR("Socket flags alınamadı: " + std::string(strerror(errno)));
            return false;
        }
        if (fcntl(sockfd_, F_SETFL, flags | O_NONBLOCK) < 0) {
            LOG_ERROR("Non-blocking mode ayarlanamadı: " + std::string(strerror(errno)));
            return false;
        }
        
        // Bind to any available port
        struct sockaddr_in local_addr{};
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = INADDR_ANY;
        local_addr.sin_port = 0; // Let OS choose port
        
        if (bind(sockfd_, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
            LOG_ERROR("Socket bind edilemedi: " + std::string(strerror(errno)));
            return false;
        }
        
        // Setup remote address
        memset(&remote_addr_, 0, sizeof(remote_addr_));
        remote_addr_.sin_family = AF_INET;
        remote_addr_.sin_port = htons(remote_port_);
        
        if (inet_pton(AF_INET, remote_ip_.c_str(), &remote_addr_.sin_addr) <= 0) {
            LOG_ERROR("Geçersiz IP adresi: " + remote_ip_);
            return false;
        }
        
        // Get local port
        socklen_t addr_len = sizeof(local_addr);
        if (getsockname(sockfd_, (struct sockaddr*)&local_addr, &addr_len) < 0) {
            LOG_ERROR("Local port alınamadı: " + std::string(strerror(errno)));
            return false;
        }
        local_port_ = ntohs(local_addr.sin_port);
        
        LOG_INFO("SenderReceiver başlatıldı: " + remote_ip_ + ":" + std::to_string(remote_port_) + 
                 " (local port: " + std::to_string(local_port_) + ")");
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("SenderReceiver başlatma hatası: " + std::string(e.what()));
        return false;
    }
}

void SenderReceiver::start() {
    if (running_.load()) {
        LOG_WARNING("SenderReceiver zaten çalışıyor");
        return;
    }
    
    running_ = true;
    receive_thread_ = std::thread(&SenderReceiver::receive_loop, this);
    
    LOG_INFO("SenderReceiver başlatıldı");
}

void SenderReceiver::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_ = false;
    
    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }
    
    LOG_INFO("SenderReceiver durduruldu");
}

void SenderReceiver::send_chunk(const std::vector<uint8_t>& chunk_data) {
    if (sockfd_ < 0 || !running_.load()) {
        return;
    }
    
    try {
        ssize_t bytes_sent = sendto(sockfd_, chunk_data.data(), chunk_data.size(), 0,
                                   (const struct sockaddr*)&remote_addr_, sizeof(remote_addr_));
        
        if (bytes_sent < 0) {
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                LOG_ERROR("Chunk gönderilemedi: " + std::string(strerror(errno)));
            }
        } else if (bytes_sent != static_cast<ssize_t>(chunk_data.size())) {
            LOG_WARNING("Kısmi gönderim: " + std::to_string(bytes_sent) + "/" + std::to_string(chunk_data.size()));
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Chunk gönderme hatası: " + std::string(e.what()));
    }
}

std::vector<std::vector<uint8_t>> SenderReceiver::receive_chunks() {
    std::vector<std::vector<uint8_t>> received_chunks;
    
    if (sockfd_ < 0) {
        return received_chunks;
    }
    
    std::vector<uint8_t> buffer(65536); // Large buffer for video chunks
    
    while (running_.load()) {
        struct sockaddr_in src_addr{};
        socklen_t addr_len = sizeof(src_addr);
        
        ssize_t bytes_read = recvfrom(sockfd_, buffer.data(), buffer.size(), 0,
                                     (struct sockaddr*)&src_addr, &addr_len);
        
        if (bytes_read > 0) {
            // Verify sender
            if (src_addr.sin_addr.s_addr == remote_addr_.sin_addr.s_addr &&
                src_addr.sin_port == remote_addr_.sin_port) {
                
                std::vector<uint8_t> chunk_data(buffer.begin(), buffer.begin() + bytes_read);
                received_chunks.push_back(std::move(chunk_data));
            }
        } else if (bytes_read < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // No data available, break
                break;
            } else {
                LOG_ERROR("Alma hatası: " + std::string(strerror(errno)));
                break;
            }
        } else {
            // Connection closed
            break;
        }
    }
    
    return received_chunks;
}

void SenderReceiver::receive_loop() {
    while (running_.load()) {
        try {
            auto chunks = receive_chunks();
            
            if (!chunks.empty()) {
                std::lock_guard<std::mutex> lock(received_chunks_mutex_);
                received_chunks_.insert(received_chunks_.end(), 
                                      std::make_move_iterator(chunks.begin()),
                                      std::make_move_iterator(chunks.end()));
            }
            
        } catch (const std::exception& e) {
            LOG_ERROR("Alma döngüsü hatası: " + std::string(e.what()));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

std::vector<std::vector<uint8_t>> SenderReceiver::get_received_chunks() {
    std::lock_guard<std::mutex> lock(received_chunks_mutex_);
    std::vector<std::vector<uint8_t>> chunks;
    chunks.swap(received_chunks_);
    return chunks;
}

std::string SenderReceiver::get_remote_ip() const {
    return remote_ip_;
}

uint16_t SenderReceiver::get_remote_port() const {
    return remote_port_;
}

uint16_t SenderReceiver::get_local_port() const {
    return local_port_;
}

bool SenderReceiver::is_running() const {
    return running_.load();
}