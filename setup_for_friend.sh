#!/bin/bash

# Nova Engine V3 - Arkadaşınız için Kurulum Scripti
echo "=== Nova Engine V3 - Arkadaşınız için Kurulum ==="

# Renkli çıktı için
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}1. Bağımlılıklar kontrol ediliyor...${NC}"

# Temel araçları kontrol et
if ! command -v cmake &> /dev/null; then
    echo -e "${YELLOW}CMake bulunamadı. Kuruluyor...${NC}"
    sudo apt update
    sudo apt install -y cmake build-essential pkg-config
else
    echo -e "${GREEN}✓ CMake mevcut${NC}"
fi

# OpenCV kontrol et
if ! pkg-config --exists opencv4; then
    echo -e "${YELLOW}OpenCV bulunamadı. Kuruluyor...${NC}"
    sudo apt install -y libopencv-dev
else
    echo -e "${GREEN}✓ OpenCV mevcut${NC}"
fi

# FFmpeg kontrol et (opsiyonel)
if ! pkg-config --exists libavcodec; then
    echo -e "${YELLOW}FFmpeg bulunamadı. Kuruluyor...${NC}"
    sudo apt install -y libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
else
    echo -e "${GREEN}✓ FFmpeg mevcut${NC}"
fi

echo -e "${GREEN}2. Proje build ediliyor...${NC}"

# Build dizini oluştur
if [ -d "build" ]; then
    echo "Eski build dizini siliniyor..."
    rm -rf build
fi

mkdir build
cd build

# CMake ile configure et
echo "CMake ile configure ediliyor..."
cmake .. > cmake_output.log 2>&1
if [ $? -ne 0 ]; then
    echo -e "${RED}CMake hatası! Log dosyasını kontrol edin: build/cmake_output.log${NC}"
    exit 1
fi

# Sadece çalışan uygulamaları build et
echo "Uygulamalar build ediliyor..."
make video_chat udp_chat udp_test udp_test_friend -j$(nproc) > build_output.log 2>&1
if [ $? -ne 0 ]; then
    echo -e "${RED}Build hatası! Log dosyasını kontrol edin: build/build_output.log${NC}"
    exit 1
fi

echo -e "${GREEN}3. Test ediliyor...${NC}"

# Uygulamaların varlığını kontrol et
if [ -f "video_chat" ] && [ -f "udp_chat" ] && [ -f "udp_test" ] && [ -f "udp_test_friend" ]; then
    echo -e "${GREEN}✓ Tüm uygulamalar başarıyla build edildi!${NC}"
else
    echo -e "${RED}✗ Bazı uygulamalar build edilemedi${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}=== Kurulum Tamamlandı! ===${NC}"
echo ""
echo -e "${YELLOW}Kullanılabilir uygulamalar:${NC}"
echo "  • ./video_chat     - Görüntülü video chat"
echo "  • ./udp_chat       - Metin tabanlı UDP chat"
echo "  • ./udp_test       - UDP test uygulaması"
echo "  • ./udp_test_friend - UDP test uygulaması (arkadaş)"
echo ""
echo -e "${YELLOW}Kullanım örneği:${NC}"
echo "  ./video_chat"
echo "  ./udp_chat"
echo ""
echo -e "${GREEN}Artık görüntülü iletişim kurabilirsiniz! 🎉${NC}" 