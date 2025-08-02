// src/core/main_friend.cpp - Arkadaşınız için
#include "engine.h"
#include "../common/logger.h"
#include <iostream>
#include <stdexcept>

int main() {
    try {
        // Engine konfigürasyonu
        EngineConfig config;
        config.width = 1280;
        config.height = 720;
        config.fps = 30;
        config.bitrate_kbps = 3000;
        config.max_chunk_size = 1000;
        config.k_chunks = 8;
        config.r_chunks = 2;
        config.jitter_buffer_ms = 100;
        
        // Path konfigürasyonu
        config.paths.emplace_back("192.168.1.254", 45000);
        
        // Engine'i başlat
        Engine nova_engine(config);
        nova_engine.start();
        
        // Ana döngü
        std::cout << "Nova Engine V3 başlatıldı. Çıkmak için Ctrl+C kullanın." << std::endl;
        
        // Basit bir bekleme döngüsü
        while (nova_engine.is_running()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Kritik hata oluştu: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
} 