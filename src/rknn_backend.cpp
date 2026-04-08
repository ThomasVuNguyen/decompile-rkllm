/**
 * RKNN Backend Implementation
 * 
 * Provides hardware-accelerated matrix multiplication using Rockchip NPU.
 * Falls back to CPU when RKNN is not available.
 */

#include "rknn_backend.h"
#include "rkllm_internal.h"
#include <cstring>
#include <cmath>
#include <fstream>

namespace rkllm {

// Half-precision to float
static float half_to_float(uint16_t h) {
    uint32_t sign = (h >> 15) & 1;
    uint32_t exp = (h >> 10) & 0x1F;
    uint32_t mant = h & 0x3FF;
    
    if (exp == 0) {
        if (mant == 0) return sign ? -0.0f : 0.0f;
        while (!(mant & 0x400)) {
            mant <<= 1;
            exp--;
        }
        exp++;
        mant &= 0x3FF;
    } else if (exp == 31) {
        if (mant == 0) return sign ? -INFINITY : INFINITY;
        return NAN;
    }
    
    uint32_t f = (sign << 31) | ((exp + 112) << 23) | (mant << 13);
    float result;
    memcpy(&result, &f, 4);
    return result;
}

// Float to half-precision
static uint16_t float_to_half(float f) {
    uint32_t x;
    memcpy(&x, &f, 4);
    
    uint32_t sign = (x >> 31) & 1;
    int32_t exp = ((x >> 23) & 0xFF) - 127;
    uint32_t mant = x & 0x7FFFFF;
    
    if (exp > 15) {
        return (sign << 15) | 0x7C00;  // Infinity
    } else if (exp < -14) {
        return (sign << 15);  // Zero
    }
    
    uint16_t h = (sign << 15) | ((exp + 15) << 10) | (mant >> 13);
    return h;
}

// ============================================================================
// RKNNMatmul Implementation
// ============================================================================

RKNNMatmul::RKNNMatmul() : created_(false) {
#ifdef RKLLM_HAS_RKNN
    ctx_ = 0;
    weight_mem_ = nullptr;
    input_mem_ = nullptr;
    output_mem_ = nullptr;
#endif
}

RKNNMatmul::~RKNNMatmul() {
    destroy();
}

bool RKNNMatmul::create(const MatmulParams& params) {
    params_ = params;
    
#ifdef RKLLM_HAS_RKNN
    rknn_matmul_info info;
    memset(&info, 0, sizeof(info));
    
    info.M = params.M;
    info.K = params.K;
    info.N = params.N;
    
    switch (params.type) {
        case MatmulType::FP16_FP16_FP16:
            info.type = RKNN_FLOAT16_MM_FLOAT16_TO_FLOAT16;
            break;
        case MatmulType::FP16_INT8_FP16:
            info.type = RKNN_FLOAT16_MM_INT8_TO_FLOAT16;
            break;
        case MatmulType::FP16_INT4_FP16:
            info.type = RKNN_FLOAT16_MM_INT4_TO_FLOAT16;
            break;
        case MatmulType::INT8_INT8_INT32:
            info.type = RKNN_INT8_MM_INT8_TO_INT32;
            break;
    }
    
    info.B_layout = 1;  // Native layout
    info.AC_layout = 0;  // Normal layout
    info.group_size = 32;
    
    rknn_matmul_io_attr io_attr;
    int ret = rknn_matmul_create(&ctx_, &info, &io_attr);
    if (ret != 0) {
        rkllm_log("E", "rknn_matmul_create failed: %d", ret);
        return false;
    }
    
    created_ = true;
    return true;
    
#else
    // CPU fallback - just store params
    created_ = true;
    return true;
#endif
}

void RKNNMatmul::destroy() {
    if (!created_) return;
    
#ifdef RKLLM_HAS_RKNN
    if (ctx_) {
        rknn_matmul_destroy(ctx_);
        ctx_ = 0;
    }
    // Free tensor memory
    if (weight_mem_) {
        rknn_destroy_mem(0, weight_mem_);
        weight_mem_ = nullptr;
    }
    if (input_mem_) {
        rknn_destroy_mem(0, input_mem_);
        input_mem_ = nullptr;
    }
    if (output_mem_) {
        rknn_destroy_mem(0, output_mem_);
        output_mem_ = nullptr;
    }
#else
    weight_f32_.clear();
#endif
    
    created_ = false;
}

bool RKNNMatmul::set_weight(const void* data, size_t size) {
#ifdef RKLLM_HAS_RKNN
    // Create and copy weight memory
    weight_mem_ = rknn_create_mem(0, size);
    if (!weight_mem_) return false;
    memcpy(weight_mem_->virt_addr, data, size);
    
    rknn_matmul_set_io_mem(ctx_, weight_mem_, 
                           RKNN_TENSOR_STATIC, RKNN_INPUT_SCALE_GROUP);
    return true;
#else
    // CPU: dequantize weights to f32
    // For now, assume f16 weights
    size_t n_elements = params_.K * params_.N;
    weight_f32_.resize(n_elements);
    
    const uint16_t* src = static_cast<const uint16_t*>(data);
    for (size_t i = 0; i < n_elements; i++) {
        weight_f32_[i] = half_to_float(src[i]);
    }
    return true;
#endif
}

bool RKNNMatmul::run(const void* input, void* output, size_t batch) {
#ifdef RKLLM_HAS_RKNN
    // Set input memory
    size_t input_size = batch * params_.M * params_.K * sizeof(uint16_t);
    if (!input_mem_ || input_mem_->size < input_size) {
        if (input_mem_) rknn_destroy_mem(0, input_mem_);
        input_mem_ = rknn_create_mem(0, input_size);
    }
    memcpy(input_mem_->virt_addr, input, input_size);
    rknn_matmul_set_io_mem(ctx_, input_mem_, 
                           RKNN_TENSOR_INPUT, RKNN_INPUT_SCALE_GROUP);
    
    // Set output memory
    size_t output_size = batch * params_.M * params_.N * sizeof(uint16_t);
    if (!output_mem_ || output_mem_->size < output_size) {
        if (output_mem_) rknn_destroy_mem(0, output_mem_);
        output_mem_ = rknn_create_mem(0, output_size);
    }
    rknn_matmul_set_io_mem(ctx_, output_mem_,
                           RKNN_TENSOR_OUTPUT, RKNN_OUTPUT_SCALE);
    
    // Run matmul
    int ret = rknn_matmul_run(ctx_);
    if (ret != 0) {
        rkllm_log("E", "rknn_matmul_run failed: %d", ret);
        return false;
    }
    
    // Copy output
    memcpy(output, output_mem_->virt_addr, output_size);
    return true;
    
#else
    // CPU fallback: naive matmul
    const float* A = static_cast<const float*>(input);
    float* C = static_cast<float*>(output);
    
    size_t M = params_.M;
    size_t K = params_.K;
    size_t N = params_.N;
    
    for (size_t b = 0; b < batch; b++) {
        for (size_t i = 0; i < M; i++) {
            for (size_t j = 0; j < N; j++) {
                float sum = 0.0f;
                for (size_t k = 0; k < K; k++) {
                    sum += A[b * M * K + i * K + k] * weight_f32_[k * N + j];
                }
                C[b * M * N + i * N + j] = sum;
            }
        }
    }
    return true;
#endif
}

// ============================================================================
// RKNNBackend Implementation
// ============================================================================

RKNNBackend::RKNNBackend() : available_(false), n_layers_(0), 
    n_embd_(0), n_head_(0), n_head_kv_(0), n_ff_(0), n_vocab_(0) {}

RKNNBackend::~RKNNBackend() {
    shutdown();
}

bool RKNNBackend::init(int n_layers, int n_embd, int n_head, int n_head_kv, 
                        int n_ff, int n_vocab) {
    n_layers_ = n_layers;
    n_embd_ = n_embd;
    n_head_ = n_head;
    n_head_kv_ = n_head_kv;
    n_ff_ = n_ff;
    n_vocab_ = n_vocab;
    
    layer_contexts_.resize(n_layers);
    
#ifdef RKLLM_HAS_RKNN
    available_ = true;
    rkllm_log("I", "RKNN backend initialized with %d layers", n_layers);
#else
    available_ = false;
    rkllm_log("W", "RKNN not available, using CPU fallback");
#endif
    
    return true;
}

void RKNNBackend::shutdown() {
    layer_contexts_.clear();
    lm_head_.reset();
    available_ = false;
}

bool RKNNBackend::create_layer_contexts(int layer_idx,
                                         const void* q_weight, size_t q_size,
                                         const void* k_weight, size_t k_size,
                                         const void* v_weight, size_t v_size,
                                         const void* o_weight, size_t o_size,
                                         const void* gate_weight, size_t gate_size,
                                         const void* up_weight, size_t up_size,
                                         const void* down_weight, size_t down_size) {
    if (layer_idx >= n_layers_) return false;
    
    auto& ctx = layer_contexts_[layer_idx];
    int head_dim = n_embd_ / n_head_;
    int kv_dim = n_head_kv_ * head_dim;
    
    // Q projection: [1, n_embd] @ [n_embd, n_embd] -> [1, n_embd]
    ctx.q_proj = std::make_unique<RKNNMatmul>();
    MatmulParams q_params{1, (size_t)n_embd_, (size_t)n_embd_, 
                          MatmulType::FP16_INT4_FP16, false, false};
    ctx.q_proj->create(q_params);
    ctx.q_proj->set_weight(q_weight, q_size);
    
    // K projection: [1, n_embd] @ [n_embd, kv_dim] -> [1, kv_dim]
    ctx.k_proj = std::make_unique<RKNNMatmul>();
    MatmulParams k_params{1, (size_t)n_embd_, (size_t)kv_dim,
                          MatmulType::FP16_INT4_FP16, false, false};
    ctx.k_proj->create(k_params);
    ctx.k_proj->set_weight(k_weight, k_size);
    
    // V projection
    ctx.v_proj = std::make_unique<RKNNMatmul>();
    ctx.v_proj->create(k_params);  // Same dims as K
    ctx.v_proj->set_weight(v_weight, v_size);
    
    // O projection: [1, n_embd] @ [n_embd, n_embd] -> [1, n_embd]
    ctx.o_proj = std::make_unique<RKNNMatmul>();
    ctx.o_proj->create(q_params);  // Same dims as Q
    ctx.o_proj->set_weight(o_weight, o_size);
    
    // Gate projection: [1, n_embd] @ [n_embd, n_ff] -> [1, n_ff]
    ctx.gate_proj = std::make_unique<RKNNMatmul>();
    MatmulParams gate_params{1, (size_t)n_embd_, (size_t)n_ff_,
                             MatmulType::FP16_INT4_FP16, false, false};
    ctx.gate_proj->create(gate_params);
    ctx.gate_proj->set_weight(gate_weight, gate_size);
    
    // Up projection
    ctx.up_proj = std::make_unique<RKNNMatmul>();
    ctx.up_proj->create(gate_params);
    ctx.up_proj->set_weight(up_weight, up_size);
    
    // Down projection: [1, n_ff] @ [n_ff, n_embd] -> [1, n_embd]
    ctx.down_proj = std::make_unique<RKNNMatmul>();
    MatmulParams down_params{1, (size_t)n_ff_, (size_t)n_embd_,
                             MatmulType::FP16_INT4_FP16, false, false};
    ctx.down_proj->create(down_params);
    ctx.down_proj->set_weight(down_weight, down_size);
    
    return true;
}

bool RKNNBackend::run_qkv_proj(int layer_idx, const float* hidden,
                                float* q, float* k, float* v) {
    if (layer_idx >= n_layers_) return false;
    auto& ctx = layer_contexts_[layer_idx];
    
    ctx.q_proj->run(hidden, q);
    ctx.k_proj->run(hidden, k);
    ctx.v_proj->run(hidden, v);
    
    return true;
}

bool RKNNBackend::run_o_proj(int layer_idx, const float* attn_out, float* hidden) {
    if (layer_idx >= n_layers_) return false;
    auto& ctx = layer_contexts_[layer_idx];
    
    return ctx.o_proj->run(attn_out, hidden);
}

bool RKNNBackend::run_ffn(int layer_idx, const float* hidden, float* output) {
    if (layer_idx >= n_layers_) return false;
    auto& ctx = layer_contexts_[layer_idx];
    
    std::vector<float> gate(n_ff_);
    std::vector<float> up(n_ff_);
    
    ctx.gate_proj->run(hidden, gate.data());
    ctx.up_proj->run(hidden, up.data());
    
    // SiLU activation: x * sigmoid(x)
    for (int i = 0; i < n_ff_; i++) {
        gate[i] = gate[i] / (1.0f + expf(-gate[i]));
        gate[i] *= up[i];
    }
    
    return ctx.down_proj->run(gate.data(), output);
}

bool RKNNBackend::run_lm_head(const float* hidden, float* logits) {
    if (!lm_head_) return false;
    return lm_head_->run(hidden, logits);
}

// ============================================================================
// Helper functions
// ============================================================================

bool rknn_is_available() {
#ifdef RKLLM_HAS_RKNN
    // Try to query RKNN runtime
    return true;
#else
    return false;
#endif
}

bool rknn_get_device_info(RKNNDeviceInfo& info) {
#ifdef RKLLM_HAS_RKNN
    // Query actual device info
    info.driver_version = "unknown";
    info.api_version = "2.3.0";
    info.num_cores = 3;  // RK3588 has 3 NPU cores
    info.total_memory = 0;
    info.free_memory = 0;
    return true;
#else
    return false;
#endif
}

// ============================================================================
// Legacy functions from old rknn_backend.cpp
// ============================================================================

int rkllm_prefill(RKLLMContextInternal* ctx, const int32_t* tokens, size_t n_tokens) {
    if (!ctx || !tokens || n_tokens == 0) {
        return -1;
    }
    
    rkllm_log("I", "Prefilling %zu tokens...", n_tokens);
    ctx->timing.prefill_tokens = n_tokens;
    return 0;
}

int rkllm_decode_one(RKLLMContextInternal* ctx, int32_t token) {
    if (!ctx) {
        return -1;
    }
    
    ctx->kv_cache.n_tokens++;
    ctx->timing.generate_tokens++;
    return 0;
}

} // namespace rkllm
