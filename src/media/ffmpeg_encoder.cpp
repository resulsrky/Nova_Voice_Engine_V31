// ffmpeg_encoder.cpp
#include "ffmpeg_encoder.h"

#ifdef FFMPEG_AVAILABLE
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <vector>
#include <memory>
#include <string>
#include <stdexcept>

FFmpegEncoder::FFmpegEncoder(const EncoderConfig& config) 
    : config_(config), codec_(nullptr), codec_context_(nullptr), 
      frame_(nullptr), packet_(nullptr), sws_context_(nullptr) {
}

FFmpegEncoder::~FFmpegEncoder() {
    cleanup();
}

bool FFmpegEncoder::initialize() {
    try {
        // Find encoder
        codec_ = avcodec_find_encoder_by_name(config_.codec_name.c_str());
        if (!codec_) {
            return false; // Encoder not found, but don't crash
        }
        
        // Allocate context
        codec_context_ = avcodec_alloc_context3(codec_);
        if (!codec_context_) {
            return false;
        }
        
        // Set parameters
        codec_context_->width = config_.width;
        codec_context_->height = config_.height;
        codec_context_->time_base = {1, config_.fps};
        codec_context_->framerate = {config_.fps, 1};
        codec_context_->pix_fmt = AV_PIX_FMT_YUV420P;
        codec_context_->bit_rate = config_.bitrate;
        codec_context_->gop_size = 10;
        codec_context_->max_b_frames = 0;
        
        // Open codec
        if (avcodec_open2(codec_context_, codec_, nullptr) < 0) {
            return false;
        }
        
        // Allocate frame
        if (!init_frame()) {
            return false;
        }
        
        // Allocate packet
        packet_ = av_packet_alloc();
        if (!packet_) {
            return false;
        }
        
        return true;
        
    } catch (...) {
        cleanup();
        return false;
    }
}

bool FFmpegEncoder::init_frame() {
    frame_ = av_frame_alloc();
    if (!frame_) {
        return false;
    }
    
    frame_->format = AV_PIX_FMT_YUV420P;
    frame_->width = config_.width;
    frame_->height = config_.height;
    
    if (av_frame_get_buffer(frame_, 0) < 0) {
        return false;
    }
    
    return true;
}

bool FFmpegEncoder::convert_frame(const uint8_t* input_data, int width, int height) {
    if (!sws_context_) {
        sws_context_ = sws_getContext(
            width, height, AV_PIX_FMT_RGB24,
            config_.width, config_.height, AV_PIX_FMT_YUV420P,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );
        if (!sws_context_) {
            return false;
        }
    }
    
    // Set up source data
    uint8_t* src_data[4] = {const_cast<uint8_t*>(input_data), nullptr, nullptr, nullptr};
    int src_linesize[4] = {width * 3, 0, 0, 0};
    
    // Scale and convert
    if (sws_scale(sws_context_, src_data, src_linesize, 0, height,
                   frame_->data, frame_->linesize) < 0) {
        return false;
    }
    
    return true;
}

std::vector<uint8_t> FFmpegEncoder::encode_frame(const uint8_t* frame_data, int width, int height) {
    if (!codec_context_ || !frame_ || !packet_) {
        return {};
    }
    
    try {
        // Convert frame
        if (!convert_frame(frame_data, width, height)) {
            return {};
        }
        
        // Send frame to encoder
        if (avcodec_send_frame(codec_context_, frame_) < 0) {
            return {};
        }
        
        // Receive packet
        if (avcodec_receive_packet(codec_context_, packet_) < 0) {
            return {};
        }
        
        // Copy packet data
        std::vector<uint8_t> encoded_data(packet_->data, packet_->data + packet_->size);
        
        // Unref packet
        av_packet_unref(packet_);
        
        return encoded_data;
        
    } catch (...) {
        return {};
    }
}

std::vector<std::vector<uint8_t>> FFmpegEncoder::flush() {
    std::vector<std::vector<uint8_t>> packets;
    
    if (!codec_context_ || !packet_) {
        return packets;
    }
    
    try {
        // Send NULL frame to flush
        if (avcodec_send_frame(codec_context_, nullptr) < 0) {
            return packets;
        }
        
        // Receive remaining packets
        while (avcodec_receive_packet(codec_context_, packet_) >= 0) {
            std::vector<uint8_t> packet_data(packet_->data, packet_->data + packet_->size);
            packets.push_back(packet_data);
            av_packet_unref(packet_);
        }
        
    } catch (...) {
        // Ignore errors during flush
    }
    
    return packets;
}

void FFmpegEncoder::cleanup() {
    if (sws_context_) {
        sws_freeContext(sws_context_);
        sws_context_ = nullptr;
    }
    
    if (packet_) {
        av_packet_free(&packet_);
    }
    
    if (frame_) {
        av_frame_free(&frame_);
    }
    
    if (codec_context_) {
        avcodec_free_context(&codec_context_);
    }
    
    codec_ = nullptr;
}

#else
// FFmpeg yoksa dummy implementation
FFmpegEncoder::FFmpegEncoder(const EncoderConfig& config) {}
FFmpegEncoder::~FFmpegEncoder() {}
bool FFmpegEncoder::initialize() { return false; }
std::vector<uint8_t> FFmpegEncoder::encode_frame(const uint8_t*, int, int) { return {}; }
std::vector<std::vector<uint8_t>> FFmpegEncoder::flush() { return {}; }
void FFmpegEncoder::cleanup() {}
bool FFmpegEncoder::init_frame() { return false; }
bool FFmpegEncoder::convert_frame(const uint8_t*, int, int) { return false; }
#endif