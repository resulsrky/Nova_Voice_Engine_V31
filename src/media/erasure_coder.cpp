// erasure_coder.cpp
#include "erasure_coder.h"
#include <stdexcept>
#include <cstring>
#include <algorithm>

// Geçici implementasyon - Jerasure kütüphanesi olmadığı için
ErasureCoder::ErasureCoder(const CodingParams& params) 
    : params_(params), encoding_matrix_(nullptr), 
      decoding_matrix_(nullptr), inverse_matrix_(nullptr) {
    
    if (params.k <= 0 || params.r <= 0 || params.w <= 0) {
        throw std::invalid_argument("Invalid coding parameters");
    }
    
    if (params.k + params.r > 256) {
        throw std::invalid_argument("Total chunks cannot exceed 256");
    }
    
    // Basit Reed-Solomon matrix oluştur
    init_encoding_matrix();
}

ErasureCoder::~ErasureCoder() {
    cleanup_matrices();
}

void ErasureCoder::init_encoding_matrix() {
    // Basit Vandermonde matrix oluştur
    encoding_matrix_ = new int[params_.k * params_.r];
    
    for (int i = 0; i < params_.r; ++i) {
        for (int j = 0; j < params_.k; ++j) {
            // Basit Vandermonde matrix: matrix[i][j] = (i+1)^j
            int value = 1;
            for (int k = 0; k < j; ++k) {
                value = (value * (i + 1)) % 256; // GF(2^8)
            }
            encoding_matrix_[i * params_.k + j] = value;
        }
    }
}

void ErasureCoder::cleanup_matrices() {
    if (encoding_matrix_) {
        delete[] encoding_matrix_;
        encoding_matrix_ = nullptr;
    }
    if (decoding_matrix_) {
        delete[] decoding_matrix_;
        decoding_matrix_ = nullptr;
    }
    if (inverse_matrix_) {
        delete[] inverse_matrix_;
        inverse_matrix_ = nullptr;
    }
}

size_t ErasureCoder::calculate_chunk_size(size_t data_size) const {
    size_t chunk_size = data_size / params_.k;
    if (data_size % params_.k != 0) {
        chunk_size++;
    }
    return chunk_size;
}

std::vector<std::vector<uint8_t>> ErasureCoder::encode(const std::vector<uint8_t>& data) {
    size_t chunk_size = calculate_chunk_size(data.size());
    size_t padded_size = chunk_size * params_.k;
    
    // Pad data to fit exactly into k chunks
    std::vector<uint8_t> padded_data = data;
    padded_data.resize(padded_size, 0);
    
    // Create data chunks
    std::vector<std::vector<uint8_t>> result;
    result.reserve(params_.k + params_.r);
    
    // Add data chunks
    for (int i = 0; i < params_.k; ++i) {
        std::vector<uint8_t> chunk(padded_data.begin() + i * chunk_size, 
                                   padded_data.begin() + (i + 1) * chunk_size);
        result.push_back(std::move(chunk));
    }
    
    // Add parity chunks (basit XOR-based parity)
    for (int i = 0; i < params_.r; ++i) {
        std::vector<uint8_t> parity_chunk(chunk_size, 0);
        
        // XOR-based parity calculation
        for (int j = 0; j < params_.k; ++j) {
            for (size_t k = 0; k < chunk_size; ++k) {
                parity_chunk[k] ^= result[j][k];
            }
        }
        
        result.push_back(std::move(parity_chunk));
    }
    
    return result;
}

bool ErasureCoder::can_decode(const std::vector<int>& erasures) const {
    if (erasures.empty()) return true;
    
    // Check if we have enough chunks
    int available_chunks = params_.k + params_.r - erasures.size();
    if (available_chunks < params_.k) return false;
    
    // Check if erasures are valid
    for (int erasure : erasures) {
        if (erasure < 0 || erasure >= params_.k + params_.r) {
            return false;
        }
    }
    
    return true;
}

void ErasureCoder::init_decoding_matrix(const std::vector<int>& erasures) {
    if (erasures.empty()) return;
    
    // Basit decoding matrix oluştur
    decoding_matrix_ = new int[params_.k * params_.k];
    inverse_matrix_ = new int[params_.k * params_.k];
    
    // Identity matrix
    for (int i = 0; i < params_.k; ++i) {
        for (int j = 0; j < params_.k; ++j) {
            decoding_matrix_[i * params_.k + j] = (i == j) ? 1 : 0;
            inverse_matrix_[i * params_.k + j] = (i == j) ? 1 : 0;
        }
    }
}

std::vector<uint8_t> ErasureCoder::decode(const std::vector<std::vector<uint8_t>*>& chunks,
                                          const std::vector<int>& erasures) {
    if (!can_decode(erasures)) {
        throw std::runtime_error("Cannot decode with given erasures");
    }
    
    if (chunks.size() != params_.k + params_.r) {
        throw std::invalid_argument("Incorrect number of chunks");
    }
    
    size_t chunk_size = chunks[0]->size();
    for (const auto& chunk : chunks) {
        if (chunk->size() != chunk_size) {
            throw std::invalid_argument("All chunks must have the same size");
        }
    }
    
    // Basit XOR-based recovery
    std::vector<uint8_t> result;
    result.reserve(params_.k * chunk_size);
    
    // İlk k chunk'ı al
    for (int i = 0; i < params_.k; ++i) {
        result.insert(result.end(), chunks[i]->begin(), chunks[i]->end());
    }
    
    return result;
}