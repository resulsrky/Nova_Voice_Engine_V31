# 🚀 Nova Engine V3 - Hızlı Başlangıç

## Arkadaşınız için Basit Kurulum

### 1. Otomatik Kurulum (Önerilen)

```bash
# Projeyi clone et
git clone <repository-url>
cd Nova_Engine_V3

# Otomatik kurulum scriptini çalıştır
./setup_for_friend.sh
```

### 2. Manuel Kurulum

```bash
# Bağımlılıkları kur
sudo apt update
sudo apt install -y cmake build-essential pkg-config libopencv-dev

# Build et
mkdir build && cd build
cmake ..
make video_chat udp_chat udp_test udp_test_friend -j$(nproc)
```

## 🎮 Kullanım

### Video Chat (Görüntülü İletişim)

```bash
cd build
./video_chat
```

**Örnek Kullanım:**
- Kendi IP: `192.168.1.5`
- Kendi Port: `45000`
- Karşı IP: `192.168.1.254`
- Karşı Port: `45001`

### UDP Chat (Metin İletişimi)

```bash
cd build
./udp_chat
```

## 🔧 Sorun Giderme

### Kamera Açılmıyor
```bash
# Kamera izinlerini kontrol et
ls -la /dev/video*

# Gerekirse kullanıcıyı video grubuna ekle
sudo usermod -a -G video $USER
# Sonra logout/login yap
```

### Port Bağlanamıyor
- Farklı port numarası deneyin
- Firewall ayarlarını kontrol edin
- Aynı portu kullanmadığınızdan emin olun

### Build Hatası
```bash
# Temiz build
rm -rf build
mkdir build && cd build
cmake ..
make video_chat udp_chat -j$(nproc)
```

## 📞 İletişim

Sorun yaşarsanız:
1. `build/cmake_output.log` dosyasını kontrol edin
2. `build/build_output.log` dosyasını kontrol edin
3. Hata mesajını paylaşın

## 🎉 Başarı!

Kurulum tamamlandıktan sonra görüntülü iletişim kurabilirsiniz! 🎥✨ 