// erasure_coder.cpp
#include "erasure_coder.h"
#include <jerasure.h>
#include <reed_sol.h>
#include <cauchy.h>
#include <stdexcept>
#include <cstring>
#include <algorithm>

ErasureCoder::ErasureCoder(const CodingParams& params) 
    : params_(params), encoding_matrix_(nullptr), 
      decoding_matrix_(nullptr), inverse_matrix_(nullptr) {
    
    if (params.k <= 0 || params.r <= 0 || params.w <= 0) {
        throw std::invalid_argument("Invalid coding parameters");
    }
    
    if (params.k + params.r > 256) {
        throw std::invalid_argument("Total chunks cannot exceed 256");
    }
    
    init_encoding_matrix();
}

ErasureCoder::~ErasureCoder() {
    cleanup_matrices();
}

void ErasureCoder::init_encoding_matrix() {
    encoding_matrix_ = reed_sol_vandermonde_coding_matrix(params_.k, params_.r, params_.w);
    if (!encoding_matrix_) {
        throw std::runtime_error("Failed to create encoding matrix");
    }
}

void ErasureCoder::cleanup_matrices() {
    if (encoding_matrix_) {
        free(encoding_matrix_);
        encoding_matrix_ = nullptr;
    }
    if (decoding_matrix_) {
        free(decoding_matrix_);
        decoding_matrix_ = nullptr;
    }
    if (inverse_matrix_) {
        free(inverse_matrix_);
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
    std::vector<uint8_t*> data_chunks(params_.k);
    for (int i = 0; i < params_.k; ++i) {
        data_chunks[i] = new uint8_t[chunk_size];
        std::memcpy(data_chunks[i], padded_data.data() + i * chunk_size, chunk_size);
    }
    
    // Create parity chunks
    std::vector<uint8_t*> parity_chunks(params_.r);
    for (int i = 0; i < params_.r; ++i) {
        parity_chunks[i] = new uint8_t[chunk_size];
    }
    
    // Encode
    jerasure_matrix_encode(params_.k, params_.r, params_.w, encoding_matrix_,
                          data_chunks.data(), parity_chunks.data(), chunk_size);
    
    // Convert to vectors
    std::vector<std::vector<uint8_t>> result;
    result.reserve(params_.k + params_.r);
    
    // Add data chunks
    for (int i = 0; i < params_.k; ++i) {
        result.emplace_back(data_chunks[i], data_chunks[i] + chunk_size);
        delete[] data_chunks[i];
    }
    
    // Add parity chunks
    for (int i = 0; i < params_.r; ++i) {
        result.emplace_back(parity_chunks[i], parity_chunks[i] + chunk_size);
        delete[] parity_chunks[i];
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
    
    // Create arrays for jerasure
    int* erasures_array = new int[erasures.size() + 1];
    std::copy(erasures.begin(), erasures.end(), erasures_array);
    erasures_array[erasures.size()] = -1;
    
    // Allocate matrices
    decoding_matrix_ = new int[params_.k * params_.k];
    inverse_matrix_ = new int[params_.k * params_.k];
    
    // Create decoding matrix
    int result = jerasure_make_decoding_matrix(params_.k, params_.r, params_.w,
                                              encoding_matrix_, erasures_array,
                                              decoding_matrix_, inverse_matrix_);
    
    delete[] erasures_array;
    
    if (result < 0) {
        cleanup_matrices();
        throw std::runtime_error("Failed to create decoding matrix");
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
    
    // Create arrays for jerasure
    uint8_t** data_chunks = new uint8_t*[params_.k];
    uint8_t** parity_chunks = new uint8_t*[params_.r];
    
    // Copy data chunks
    for (int i = 0; i < params_.k; ++i) {
        data_chunks[i] = new uint8_t[chunk_size];
        std::memcpy(data_chunks[i], (*chunks[i]).data(), chunk_size);
    }
    
    // Copy parity chunks
    for (int i = 0; i < params_.r; ++i) {
        parity_chunks[i] = new uint8_t[chunk_size];
        std::memcpy(parity_chunks[i], (*chunks[params_.k + i]).data(), chunk_size);
    }
    
    // Initialize decoding matrix if needed
    if (!erasures.empty()) {
        init_decoding_matrix(erasures);
        
        // Decode using matrix
        jerasure_matrix_decode(params_.k, params_.r, params_.w,
                              decoding_matrix_, erasures.data(),
                              data_chunks, parity_chunks, chunk_size);
    }
    
    // Reconstruct original data
    std::vector<uint8_t> result;
    result.reserve(params_.k * chunk_size);
    
    for (int i = 0; i < params_.k; ++i) {
        result.insert(result.end(), data_chunks[i], data_chunks[i] + chunk_size);
        delete[] data_chunks[i];
    }
    
    for (int i = 0; i < params_.r; ++i) {
        delete[] parity_chunks[i];
    }
    
    delete[] data_chunks;
    delete[] parity_chunks;
    
    return result;
}