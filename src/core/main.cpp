// src/core/main.cpp - Görüntülü Video Chat
#include <opencv2/opencv.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <regex>

// Video chat uygulaması - Nova Engine V3
class VideoChat {
private:
    cv::VideoCapture cap_;
    int sock_;
    std::string local_ip_;
    uint16_t local_port_;
    std::string remote_ip_;
    uint16_t remote_port_;
    std::atomic<bool> running_;
    std::thread send_thread_;
    std::thread receive_thread_;
    
    // FFmpeg preset ve tune ayarları
    std::string preset_;
    std::string tune_;

public:
    VideoChat() : sock_(-1), running_(false), preset_("veryfast"), tune_("grain") {}
    
    ~VideoChat() {
        stop();
    }
    
    bool initialize() {
        std::cout << "=== Nova Engine V3 - Video Chat ===" << std::endl;
        std::cout << "Görüntülü İletişim\n" << std::endl;
        
        // IP ve port bilgilerini al
        std::cout << "Kendi IP adresinizi girin: ";
        std::getline(std::cin, local_ip_);
        
        std::cout << "Kendi port numaranızı girin (1-65535): ";
        std::cin >> local_port_;
        std::cin.ignore();
        
        std::cout << "Karşı tarafın IP adresini girin: ";
        std::getline(std::cin, remote_ip_);
        
        std::cout << "Karşı tarafın port numarasını girin (1-65535): ";
        std::cin >> remote_port_;
        std::cin.ignore();
        
        // Preset ve tune ayarlarını al
        std::cout << "FFmpeg preset ayarı (ultrafast/fast/veryfast/superfast/medium/slow/slower/veryslow) [veryfast]: ";
        std::string preset_input;
        std::getline(std::cin, preset_input);
        if (!preset_input.empty()) {
            preset_ = preset_input;
        }
        
        std::cout << "FFmpeg tune ayarı (film/animation/grain/fastdecode/zerolatency) [grain]: ";
        std::string tune_input;
        std::getline(std::cin, tune_input);
        if (!tune_input.empty()) {
            tune_ = tune_input;
        }
        
        std::cout << "\n=== Video Chat Bağlantısı ===" << std::endl;
        std::cout << "Sizin IP:Port = " << local_ip_ << ":" << local_port_ << std::endl;
        std::cout << "Karşı taraf IP:Port = " << remote_ip_ << ":" << remote_port_ << std::endl;
        std::cout << "FFmpeg Preset: " << preset_ << std::endl;
        std::cout << "FFmpeg Tune: " << tune_ << std::endl;
        std::cout << "==============================\n" << std::endl;
        
        // Kamera başlat
        cap_.open(0);
        if (!cap_.isOpened()) {
            std::cerr << "Kamera açılamadı!" << std::endl;
            return false;
        }
        
        cap_.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        cap_.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
        cap_.set(cv::CAP_PROP_FPS, 30);
        
        // Socket oluştur
        sock_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_ < 0) {
            std::cerr << "Socket oluşturulamadı!" << std::endl;
            return false;
        }
        
        // Socket ayarları
        int opt = 1;
        setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        // Bind
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(local_port_);
        addr.sin_addr.s_addr = inet_addr(local_ip_.c_str());
        
        if (bind(sock_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "Bind hatası!" << std::endl;
            return false;
        }
        
        // Non-blocking
        fcntl(sock_, F_SETFL, O_NONBLOCK);
        
        std::cout << "=== Video Chat Başlatıldı ===" << std::endl;
        std::cout << "Local: " << local_ip_ << ":" << local_port_ << std::endl;
        std::cout << "Remote: " << remote_ip_ << ":" << remote_port_ << std::endl;
        std::cout << "FFmpeg Preset: " << preset_ << std::endl;
        std::cout << "FFmpeg Tune: " << tune_ << std::endl;
        std::cout << "============================\n" << std::endl;
        
        return true;
    }
    
    void start() {
        if (!initialize()) {
            return;
        }
        
        running_ = true;
        send_thread_ = std::thread(&VideoChat::send_loop, this);
        receive_thread_ = std::thread(&VideoChat::receive_loop, this);
        
        std::cout << "Video chat başlatıldı! ESC tuşu ile çıkın.\n" << std::endl;
        
        // Ana döngü
        cv::Mat frame;
        while (running_) {
            cap_ >> frame;
            if (frame.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            
            // Görüntüyü göster
            cv::imshow("Sizin Görüntünüz", frame);
            
            // ESC tuşu kontrolü
            int key = cv::waitKey(1);
            if (key == 27) { // ESC
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
        }
        
        stop();
    }
    
    void stop() {
        running_ = false;
        
        if (send_thread_.joinable()) {
            send_thread_.join();
        }
        
        if (receive_thread_.joinable()) {
            receive_thread_.join();
        }
        
        if (sock_ >= 0) {
            close(sock_);
        }
        
        cap_.release();
        cv::destroyAllWindows();
        
        std::cout << "\nVideo chat sonlandırıldı." << std::endl;
    }
    
private:
    void send_loop() {
        struct sockaddr_in remote_addr;
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(remote_port_);
        remote_addr.sin_addr.s_addr = inet_addr(remote_ip_.c_str());
        
        cv::Mat frame;
        while (running_) {
            cap_ >> frame;
            if (frame.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            
            // JPEG sıkıştırma (FFmpeg preset/tune ayarları simülasyonu)
            std::vector<uchar> buffer;
            std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 80};
            cv::imencode(".jpg", frame, buffer, params);
            
            // UDP ile gönder
            sendto(sock_, buffer.data(), buffer.size(), 0,
                   (struct sockaddr*)&remote_addr, sizeof(remote_addr));
            
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }
    }
    
    void receive_loop() {
        char buffer[65536];
        struct sockaddr_in sender_addr;
        socklen_t sender_len = sizeof(sender_addr);
        
        while (running_) {
            int bytes = recvfrom(sock_, buffer, sizeof(buffer), 0,
                                (struct sockaddr*)&sender_addr, &sender_len);
            
            if (bytes > 0) {
                // JPEG decode
                std::vector<uchar> data(buffer, buffer + bytes);
                cv::Mat frame = cv::imdecode(data, cv::IMREAD_COLOR);
                
                if (!frame.empty()) {
                    cv::imshow("Karşı Taraf", frame);
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
};

bool is_valid_ip(const std::string& ip) {
    std::regex ip_regex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    return std::regex_match(ip, ip_regex);
}

bool is_valid_port(uint16_t port) {
    return port > 0 && port < 65536;
}

uint16_t get_port_input(const std::string& prompt) {
    uint16_t port;
    while (true) {
        std::cout << prompt;
        std::string input;
        std::getline(std::cin, input);
        
        try {
            port = std::stoi(input);
            if (is_valid_port(port)) {
                return port;
            } else {
                std::cout << "Hata: Port 1-65535 arasında olmalıdır.\n";
            }
        } catch (const std::exception&) {
            std::cout << "Hata: Geçerli bir sayı girin.\n";
        }
    }
}

std::string get_ip_input(const std::string& prompt) {
    std::string ip;
    while (true) {
        std::cout << prompt;
        std::getline(std::cin, ip);
        
        if (is_valid_ip(ip)) {
            return ip;
        } else {
            std::cout << "Hata: Geçerli bir IP adresi girin (örn: 192.168.1.100)\n";
        }
    }
}

int main() {
    VideoChat chat;
    chat.start();
    return 0;
}