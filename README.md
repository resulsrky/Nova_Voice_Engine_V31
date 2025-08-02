# Nova Engine V3

Gelişmiş video iletişim motoru - UDP tabanlı, düşük gecikme süreli, FEC destekli video streaming.

## 🚀 Özellikler

- **Düşük Gecikme Süresi**: UDP tabanlı hızlı video iletimi
- **Forward Error Correction (FEC)**: Paket kayıplarını telafi eden Reed-Solomon kodlama
- **Adaptif Path Seçimi**: RTT ve paket kaybına göre en iyi yol seçimi
- **Jitter Buffer**: Ağ gecikmelerini dengeleyen akıllı tampon
- **Çoklu Path Desteği**: Paralel ağ yolları ile yedeklilik
- **Gerçek Zamanlı Video**: OpenCV ile kamera yakalama ve görüntüleme

## 📋 Gereksinimler

### Zorunlu Bağımlılıklar
- **OpenCV 4.x**: Video işleme için
- **CMake 3.16+**: Build sistemi
- **C++17**: Modern C++ özellikleri
- **pkg-config**: Kütüphane bulma

### Opsiyonel Bağımlılıklar
- **FFmpeg**: Video encoding/decoding (otomatik algılanır)
- **Jerasure**: Reed-Solomon FEC (otomatik algılanır)

## 🛠️ Kurulum

### 1. Otomatik Kurulum (Önerilen)

```bash
# Bağımlılıkları otomatik kur
./install_dependencies.sh

# Projeyi build et
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 2. Manuel Kurulum

```bash
# Sistem güncellemesi
sudo apt update

# Temel araçlar
sudo apt install -y build-essential cmake pkg-config git

# OpenCV (zorunlu)
sudo apt install -y libopencv-dev

# FFmpeg (opsiyonel)
sudo apt install -y libavcodec-dev libavformat-dev libavutil-dev libswscale-dev

# Jerasure (opsiyonel)
sudo apt install -y libjerasure-dev

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 🎮 Kullanım

### Video Chat Uygulaması

```bash
cd build
./video_chat
```

Manuel IP ve port girişi ile görüntülü video chat.

### UDP Chat Uygulaması

```bash
cd build
./udp_chat
```

Metin tabanlı UDP chat uygulaması.

### UDP Test Uygulaması

```bash
cd build
./udp_test
```

Basit UDP bağlantı testi.

## 🔧 Yapılandırma

### Engine Konfigürasyonu

```cpp
EngineConfig config;
config.width = 1280;           // Video genişliği
config.height = 720;           // Video yüksekliği
config.fps = 30;              // FPS
config.bitrate_kbps = 3000;   // Bitrate
config.max_chunk_size = 1000; // Maksimum chunk boyutu
config.k_chunks = 8;          // Data chunk sayısı
config.r_chunks = 2;          // Parity chunk sayısı
config.jitter_buffer_ms = 100; // Jitter buffer süresi

// Path konfigürasyonu
config.paths.emplace_back("192.168.1.100", 45000);
```

## 📁 Proje Yapısı

```
Nova_Engine_V3/
├── src/
│   ├── core/           # Ana motor
│   ├── media/          # Video işleme
│   ├── network/        # Ağ iletişimi
│   ├── transport/      # Veri taşıma
│   └── common/         # Ortak bileşenler
├── build/              # Build çıktıları
├── tests/              # Test dosyaları
├── libs/               # Harici kütüphaneler
├── CMakeLists.txt      # Build konfigürasyonu
├── install_dependencies.sh # Kurulum scripti
└── README.md           # Bu dosya
```

## 🐛 Sorun Giderme

### FFmpeg Bulunamadı
```bash
sudo apt install -y libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
```

### OpenCV Bulunamadı
```bash
sudo apt install -y libopencv-dev
```

### CMake Hatası
```bash
sudo apt install -y cmake build-essential
```

### Build Hatası
```bash
# Temiz build
rm -rf build
mkdir build && cd build
cmake ..
make clean
make -j$(nproc)
```

## 🔍 Hata Ayıklama

### Log Seviyeleri
- `LOG_DEBUG`: Detaylı debug bilgileri
- `LOG_INFO`: Genel bilgiler
- `LOG_WARNING`: Uyarılar
- `LOG_ERROR`: Hatalar

### Performans İzleme
```bash
# CPU kullanımı
htop

# Ağ trafiği
iftop

# Disk I/O
iotop
```

## 🤝 Katkıda Bulunma

1. Fork yapın
2. Feature branch oluşturun (`git checkout -b feature/amazing-feature`)
3. Commit yapın (`git commit -m 'Add amazing feature'`)
4. Push yapın (`git push origin feature/amazing-feature`)
5. Pull Request oluşturun

## 📄 Lisans

Bu proje MIT lisansı altında lisanslanmıştır.

## 📞 İletişim

Sorularınız için issue açabilir veya pull request gönderebilirsiniz.

---

**Not**: Bu proje eğitim amaçlıdır. Üretim ortamında kullanmadan önce güvenlik testleri yapılmalıdır.
