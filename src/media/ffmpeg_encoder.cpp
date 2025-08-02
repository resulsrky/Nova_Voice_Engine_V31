// ffmpeg_encoder.cpp
#include "ffmpeg_encoder.h"
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <stdexcept>
#include <cstring>

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
            throw std::runtime_error("Encoder bulunamadı: " + config_.codec_name);
        }
        
        // Allocate codec context
        codec_context_ = avcodec_alloc_context3(codec_);
        if (!codec_context_) {
            throw std::runtime_error("Codec context oluşturulamadı");
        }
        
        // Set parameters
        codec_context_->width = config_.width;
        codec_context_->height = config_.height;
        codec_context_->time_base = {1, config_.fps};
        codec_context_->framerate = {config_.fps, 1};
        codec_context_->pix_fmt = AV_PIX_FMT_YUV420P;
        codec_context_->bit_rate = config_.bitrate * 1000; // Convert to bits per second
        
        // Set codec-specific options
        if (codec_->id == AV_CODEC_ID_H264) {
            av_opt_set(codec_context_->priv_data, "preset", "ultrafast", 0);
            av_opt_set(codec_context_->priv_data, "tune", "zerolatency", 0);
        }
        
        // Open codec
        int ret = avcodec_open2(codec_context_, codec_, nullptr);
        if (ret < 0) {
            throw std::runtime_error("Codec açılamadı");
        }
        
        // Allocate frame
        if (!init_frame()) {
            throw std::runtime_error("Frame oluşturulamadı");
        }
        
        // Allocate packet
        packet_ = av_packet_alloc();
        if (!packet_) {
            throw std::runtime_error("Packet oluşturulamadı");
        }
        
        return true;
        
    } catch (const std::exception& e) {
        cleanup();
        return false;
    }
}

bool FFmpegEncoder::init_frame() {
    frame_ = av_frame_alloc();
    if (!frame_) return false;
    
    frame_->format = AV_PIX_FMT_YUV420P;
    frame_->width = config_.width;
    frame_->height = config_.height;
    
    int ret = av_frame_get_buffer(frame_, 0);
    if (ret < 0) return false;
    
    return true;
}

bool FFmpegEncoder::convert_frame(const uint8_t* input_data, int width, int height) {
    if (!sws_context_) {
        sws_context_ = sws_getContext(width, height, AV_PIX_FMT_RGB24,
                                     config_.width, config_.height, AV_PIX_FMT_YUV420P,
                                     SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!sws_context_) return false;
    }
    
    // Set up source frame
    uint8_t* src_data[4] = {const_cast<uint8_t*>(input_data), nullptr, nullptr, nullptr};
    int src_linesize[4] = {width * 3, 0, 0, 0}; // RGB24 = 3 bytes per pixel
    
    // Set up destination frame
    uint8_t* dst_data[4] = {frame_->data[0], frame_->data[1], frame_->data[2], frame_->data[3]};
    int dst_linesize[4] = {frame_->linesize[0], frame_->linesize[1], frame_->linesize[2], frame_->linesize[3]};
    
    // Convert
    int ret = sws_scale(sws_context_, src_data, src_linesize, 0, height,
                        dst_data, dst_linesize);
    return ret > 0;
}

void FFmpegEncoder::cleanup() {
    if (sws_context_) {
        sws_freeContext(sws_context_);
        sws_context_ = nullptr;
    }
    
    if (packet_) {
        av_packet_free(&packet_);
        packet_ = nullptr;
    }
    
    if (frame_) {
        av_frame_free(&frame_);
        frame_ = nullptr;
    }
    
    if (codec_context_) {
        avcodec_free_context(&codec_context_);
        codec_context_ = nullptr;
    }
    
    codec_ = nullptr;
}

std::vector<uint8_t> FFmpegEncoder::encode_frame(const uint8_t* frame_data, int width, int height) {
    if (!is_initialized()) {
        return {};
    }
    
    try {
        // Convert frame format
        if (!convert_frame(frame_data, width, height)) {
            return {};
        }
        
        // Set frame timestamp
        static int64_t frame_count = 0;
        frame_->pts = frame_count++;
        
        // Send frame to encoder
        int ret = avcodec_send_frame(codec_context_, frame_);
        if (ret < 0) {
            return {};
        }
        
        // Receive encoded packet
        std::vector<uint8_t> encoded_data;
        while (ret >= 0) {
            ret = avcodec_receive_packet(codec_context_, packet_);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                return {};
            }
            
            // Copy packet data
            encoded_data.insert(encoded_data.end(), 
                              packet_->data, packet_->data + packet_->size);
            
            av_packet_unref(packet_);
        }
        
        return encoded_data;
        
    } catch (const std::exception&) {
        return {};
    }
}

std::vector<std::vector<uint8_t>> FFmpegEncoder::flush() {
    std::vector<std::vector<uint8_t>> packets;
    
    if (!is_initialized()) {
        return packets;
    }
    
    try {
        // Send NULL frame to flush encoder
        int ret = avcodec_send_frame(codec_context_, nullptr);
        if (ret < 0) {
            return packets;
        }
        
        // Receive remaining packets
        while (ret >= 0) {
            ret = avcodec_receive_packet(codec_context_, packet_);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                break;
            }
            
            // Copy packet data
            std::vector<uint8_t> packet_data(packet_->data, packet_->data + packet_->size);
            packets.push_back(std::move(packet_data));
            
            av_packet_unref(packet_);
        }
        
    } catch (const std::exception&) {
        // Ignore errors during flush
    }
    
    return packets;
}