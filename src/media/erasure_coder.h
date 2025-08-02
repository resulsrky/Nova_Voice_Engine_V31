// erasure_coder.h
#pragma once
#include <vector>
#include <cstdint>
#include <memory>

class ErasureCoder {
public:
    struct CodingParams {
        int k;  // Number of data chunks
        int r;  // Number of parity chunks
        int w;  // Word size (8, 16, 32)
        
        CodingParams(int k_val, int r_val, int w_val = 8) 
            : k(k_val), r(r_val), w(w_val) {}
    };
    
    ErasureCoder(const CodingParams& params);
    ~ErasureCoder();
    
    // Encode data into k data chunks and r parity chunks
    std::vector<std::vector<uint8_t>> encode(const std::vector<uint8_t>& data);
    
    // Decode data from available chunks (data + parity)
    std::vector<uint8_t> decode(const std::vector<std::vector<uint8_t>*>& chunks,
                                const std::vector<int>& erasures);
    
    // Get coding parameters
    const CodingParams& get_params() const { return params_; }
    
    // Calculate chunk size for given data size
    size_t calculate_chunk_size(size_t data_size) const;
    
    // Check if decoding is possible with given erasures
    bool can_decode(const std::vector<int>& erasures) const;

private:
    CodingParams params_;
    int* encoding_matrix_;
    int* decoding_matrix_;
    int* inverse_matrix_;
    
    // Initialize encoding matrix
    void init_encoding_matrix();
    
    // Initialize decoding matrix for given erasures
    void init_decoding_matrix(const std::vector<int>& erasures);
    
    // Free matrices
    void cleanup_matrices();
};