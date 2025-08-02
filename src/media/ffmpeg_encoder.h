// ffmpeg_encoder.h
#pragma once

// FFmpeg kütüphaneleri varsa include et
#ifdef FFMPEG_AVAILABLE
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <vector>
#include <memory>
#include <string>

class FFmpegEncoder {
public:
    struct EncoderConfig {
        int width;
        int height;
        int fps;
        int bitrate;
        std::string codec_name;
        
        EncoderConfig(int w, int h, int f, int b, const std::string& c = "libx264")
            : width(w), height(h), fps(f), bitrate(b), codec_name(c) {}
    };
    
    FFmpegEncoder(const EncoderConfig& config);
    ~FFmpegEncoder();
    
    // Initialize encoder
    bool initialize();
    
    // Encode a frame
    std::vector<uint8_t> encode_frame(const uint8_t* frame_data, int width, int height);
    
    // Flush encoder and get remaining packets
    std::vector<std::vector<uint8_t>> flush();
    
    // Get encoder configuration
    const EncoderConfig& get_config() const { return config_; }
    
    // Check if encoder is initialized
    bool is_initialized() const { return codec_context_ != nullptr; }

private:
    EncoderConfig config_;
    AVCodec* codec_;
    AVCodecContext* codec_context_;
    AVFrame* frame_;
    AVPacket* packet_;
    SwsContext* sws_context_;
    
    // Initialize frame
    bool init_frame();
    
    // Convert frame format if needed
    bool convert_frame(const uint8_t* input_data, int width, int height);
    
    // Cleanup resources
    void cleanup();
};
#else
// FFmpeg yoksa dummy class
#include <vector>
#include <string>

class FFmpegEncoder {
public:
    struct EncoderConfig {
        int width;
        int height;
        int fps;
        int bitrate;
        std::string codec_name;
        
        EncoderConfig(int w, int h, int f, int b, const std::string& c = "libx264")
            : width(w), height(h), fps(f), bitrate(b), codec_name(c) {}
    };
    
    FFmpegEncoder(const EncoderConfig& config) {}
    ~FFmpegEncoder() {}
    
    bool initialize() { return false; }
    std::vector<uint8_t> encode_frame(const uint8_t*, int, int) { return {}; }
    std::vector<std::vector<uint8_t>> flush() { return {}; }
    const EncoderConfig& get_config() const { static EncoderConfig c(0,0,0,0,""); return c; }
    bool is_initialized() const { return false; }
};
#endif