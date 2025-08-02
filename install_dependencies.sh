#!/bin/bash

# Nova Engine V3 - Bağımlılık Kurulum Scripti
echo "=== Nova Engine V3 - Bağımlılık Kurulumu ==="

# Sistem güncellemesi
echo "Sistem güncelleniyor..."
sudo apt update

# Temel geliştirme araçları
echo "Temel geliştirme araçları kuruluyor..."
sudo apt install -y build-essential cmake pkg-config

# OpenCV kurulumu
echo "OpenCV kuruluyor..."
sudo apt install -y libopencv-dev

# FFmpeg kurulumu (opsiyonel)
echo "FFmpeg kuruluyor..."
sudo apt install -y libavcodec-dev libavformat-dev libavutil-dev libswscale-dev

# Jerasure kütüphanesi (opsiyonel)
echo "Jerasure kütüphanesi kuruluyor..."
sudo apt install -y libjerasure-dev

# Git (eğer yoksa)
if ! command -v git &> /dev/null; then
    echo "Git kuruluyor..."
    sudo apt install -y git
fi

echo ""
echo "=== Kurulum Tamamlandı ==="
echo "Şimdi projeyi build edebilirsiniz:"
echo "mkdir build && cd build"
echo "cmake .."
echo "make -j$(nproc)"
echo ""
echo "Çalışan uygulamalar:"
echo "- video_chat: Görüntülü video chat"
echo "- udp_chat: Metin tabanlı UDP chat"
echo "- udp_test: UDP test uygulaması"
echo "" 