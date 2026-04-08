/**
 * RKNN NPU Backend
 * 
 * Provides hardware-accelerated matrix multiplication using Rockchip NPU.
 * Supports INT4, INT8, and FP16 quantized operations.
 */

#ifndef RKNN_BACKEND_H
#define RKNN_BACKEND_H

#include "rkllm_internal.h"
#include <vector>
#include <memory>

namespace rkllm {

// Matmul operation type
enum class MatmulType {
    FP16_FP16_FP16,         // A: FP16, B: FP16, C: FP16
    FP16_INT8_FP16,         // A: FP16, B: INT8, C: FP16
    FP16_INT4_FP16,         // A: FP16, B: INT4, C: FP16 (default for Q4 models)
    INT8_INT8_INT32,        // A: INT8, B: INT8, C: INT32
};

// Matmul parameters
struct MatmulParams {
    size_t M;       // Rows of A
    size_t K;       // Cols of A / Rows of B
    size_t N;       // Cols of B
    MatmulType type;
    bool transpose_a;
    bool transpose_b;
};

// RKNN Matmul context wrapper
class RKNNMatmul {
public:
    RKNNMatmul();
    ~RKNNMatmul();
    
    bool create(const MatmulParams& params);
    void destroy();
    
    // Set weight (B matrix) - can be done once for static weights
    bool set_weight(const void* data, size_t size);
    
    // Run matmul: C = A @ B (or A @ B^T if transpose_b)
    bool run(const void* input, void* output, size_t batch = 1);
    
private:
    MatmulParams params_;
    bool created_;
    
#ifdef RKLLM_HAS_RKNN
    rknn_matmul_ctx ctx_;
    rknn_tensor_mem* weight_mem_;
    rknn_tensor_mem* input_mem_;
    rknn_tensor_mem* output_mem_;
#else
    // CPU fallback data
    std::vector<float> weight_f32_;
#endif
};

// Main RKNN backend class
class RKNNBackend {
public:
    RKNNBackend();
    ~RKNNBackend();
    
    // Initialize backend
    bool init(int n_layers, int n_embd, int n_head, int n_head_kv, int n_ff, int n_vocab);
    void shutdown();
    
    // Create matmul contexts for model layers
    bool create_layer_contexts(int layer_idx, 
                               const void* q_weight, size_t q_size,
                               const void* k_weight, size_t k_size,
                               const void* v_weight, size_t v_size,
                               const void* o_weight, size_t o_size,
                               const void* gate_weight, size_t gate_size,
                               const void* up_weight, size_t up_size,
                               const void* down_weight, size_t down_size);
    
    // Run layer operations (returns hidden state)
    bool run_qkv_proj(int layer_idx, const float* hidden, 
                      float* q, float* k, float* v);
    bool run_o_proj(int layer_idx, const float* attn_out, float* hidden);
    bool run_ffn(int layer_idx, const float* hidden, float* output);
    bool run_lm_head(const float* hidden, float* logits);
    
    // Status
    bool is_available() const { return available_; }
    
private:
    bool available_;
    int n_layers_;
    int n_embd_;
    int n_head_;
    int n_head_kv_;
    int n_ff_;
    int n_vocab_;
    
    // Per-layer matmul contexts
    struct LayerContexts {
        std::unique_ptr<RKNNMatmul> q_proj;
        std::unique_ptr<RKNNMatmul> k_proj;
        std::unique_ptr<RKNNMatmul> v_proj;
        std::unique_ptr<RKNNMatmul> o_proj;
        std::unique_ptr<RKNNMatmul> gate_proj;
        std::unique_ptr<RKNNMatmul> up_proj;
        std::unique_ptr<RKNNMatmul> down_proj;
    };
    
    std::vector<LayerContexts> layer_contexts_;
    std::unique_ptr<RKNNMatmul> lm_head_;
};

// Check if RKNN NPU is available on this device
bool rknn_is_available();

// Get RKNN device info
struct RKNNDeviceInfo {
    std::string driver_version;
    std::string api_version;
    int num_cores;
    size_t total_memory;
    size_t free_memory;
};

bool rknn_get_device_info(RKNNDeviceInfo& info);

} // namespace rkllm

#endif // RKNN_BACKEND_H
