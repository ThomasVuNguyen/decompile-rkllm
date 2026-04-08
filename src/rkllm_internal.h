#ifndef RKLLM_INTERNAL_H
#define RKLLM_INTERNAL_H

#include "rkllm.h"
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <functional>
#include <memory>

// Forward declarations for RKNN types
#ifdef RKLLM_HAS_RKNN
#include "rknn_include/rknn_api.h"
#include "rknn_include/rknn_matmul_api.h"
#else
typedef uint64_t rknn_context;
typedef uint64_t rknn_matmul_ctx;
#endif

namespace rkllm {

// Forward declaration
class Transformer;

// Timing statistics (reverse engineered from rkllm_print_timings)
struct TimingStats {
    double load_time_ms = 0.0;
    double prefill_time_ms = 0.0;
    double prefill_extra_ms = 0.0;
    double generate_time_ms = 0.0;
    int32_t prefill_tokens = 0;
    int32_t generate_tokens = 0;
};

// Token information
struct Token {
    int32_t id;
    float logit;
    float prob;
};

// Chat template configuration
struct ChatTemplate {
    std::string system_prompt;
    std::string prompt_prefix;
    std::string prompt_postfix;
};

// Sampler state
struct SamplerState {
    std::vector<Token> candidates;
    std::vector<int32_t> last_tokens;
    int32_t repeat_last_n = 64;
    
    // Mirostat state
    float mirostat_mu = 0.0f;
};

// KV Cache state
struct KVCacheState {
    size_t n_tokens = 0;
    size_t n_capacity = 0;
    void* k_cache = nullptr;
    void* v_cache = nullptr;
};

// Model metadata
struct ModelInfo {
    std::string arch;           // qwen, llama, phi, etc.
    int32_t n_vocab = 0;
    int32_t n_ctx_train = 0;
    int32_t n_embd = 0;
    int32_t n_head = 0;
    int32_t n_head_kv = 0;
    int32_t n_layer = 0;
    int32_t n_ff = 0;
    float rope_freq_base = 10000.0f;
    float rope_freq_scale = 1.0f;
};

// Main context structure
// Size: 0x248 (584) bytes based on rkllm_init decompilation
struct RKLLMContextInternal {
    // Public params (copy of what user passed in)
    RKLLMParam params;
    
    // Callback
    LLMResultCallback callback = nullptr;
    void* userdata = nullptr;
    
    // Model state
    ModelInfo model_info;
    void* model_data = nullptr;
    size_t model_size = 0;
    
    // Inference state
    SamplerState sampler;
    KVCacheState kv_cache;
    ChatTemplate chat_template;
    
    // Timing
    TimingStats timing;
    
    // State flags
    bool is_running = false;
    bool abort_requested = false;
    bool is_async = false;
    
    // LoRA adapters
    std::vector<std::pair<std::string, float>> lora_adapters;
    
    // Prompt cache
    std::string prompt_cache_path;
    bool save_prompt_cache = false;
    
    // RKNN backend handles
    rknn_context rknn_ctx = 0;
    std::vector<rknn_matmul_ctx> matmul_contexts;
    
    // Embed flash state
    void* embed_flash_buffer = nullptr;
    
    // Result buffer
    RKLLMResult current_result;
    std::string result_text;
};

// Internal functions
int rkllm_load_model(RKLLMContextInternal* ctx, const char* model_path);
int rkllm_init_kv_cache(RKLLMContextInternal* ctx);
int rkllm_init_rknn_backend(RKLLMContextInternal* ctx);

// Inference
int rkllm_prefill(RKLLMContextInternal* ctx, const int32_t* tokens, size_t n_tokens);
int rkllm_decode_one(RKLLMContextInternal* ctx, int32_t token);
int32_t rkllm_sample(RKLLMContextInternal* ctx);

// Utilities
void rkllm_log(const char* level, const char* fmt, ...);

} // namespace rkllm

#endif // RKLLM_INTERNAL_H
