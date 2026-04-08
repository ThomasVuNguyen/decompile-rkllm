/**
 * Transformer Model Implementation
 * 
 * Core inference logic for transformer-based LLMs.
 * Supports: Llama, Qwen, Phi, Gemma, ChatGLM architectures.
 */

#ifndef TRANSFORMER_H
#define TRANSFORMER_H

#include "gguf_parser.h"
#include "rkllm_internal.h"
#include <vector>
#include <memory>

namespace rkllm {

// Forward declaration
class RKNNBackend;

// Tensor wrapper for unified access
struct Tensor {
    void* data;
    GGMLType type;
    std::vector<int64_t> shape;
    size_t n_elements;
    
    // Dequantize to float32
    void dequantize_to(float* dst) const;
    
    // Get element as float
    float get(size_t idx) const;
};

// Attention layer weights
struct AttentionWeights {
    const GGUFTensor* q;        // [n_embd, n_head * head_dim]
    const GGUFTensor* k;        // [n_embd, n_head_kv * head_dim]  
    const GGUFTensor* v;        // [n_embd, n_head_kv * head_dim]
    const GGUFTensor* o;        // [n_head * head_dim, n_embd]
    const GGUFTensor* qkv;      // Combined Q/K/V for some models
    const GGUFTensor* norm;     // Attention norm (RMSNorm/LayerNorm)
    const GGUFTensor* norm_2;   // Some models have 2 norms
    const GGUFTensor* q_norm;   // Q norm (Qwen2)
    const GGUFTensor* k_norm;   // K norm (Qwen2)
};

// FFN layer weights
struct FFNWeights {
    const GGUFTensor* gate;     // Gate projection [n_embd, n_ff]
    const GGUFTensor* up;       // Up projection [n_embd, n_ff]
    const GGUFTensor* down;     // Down projection [n_ff, n_embd]
    const GGUFTensor* norm;     // FFN norm
    
    // MoE weights (if n_expert > 0)
    const GGUFTensor* gate_inp; // Expert gate [n_embd, n_expert]
    std::vector<const GGUFTensor*> gate_exps;
    std::vector<const GGUFTensor*> up_exps;
    std::vector<const GGUFTensor*> down_exps;
};

// Layer weights
struct LayerWeights {
    AttentionWeights attn;
    FFNWeights ffn;
};

// Full model weights
struct ModelWeights {
    const GGUFTensor* token_embd;       // Token embeddings
    const GGUFTensor* token_embd_norm;  // Embedding norm (some models)
    const GGUFTensor* output_norm;      // Final RMSNorm
    const GGUFTensor* output;           // LM head (often tied to token_embd)
    std::vector<LayerWeights> layers;
};

// Inference state
struct InferenceState {
    // Working buffers (allocated once, reused)
    std::vector<float> hidden;          // [batch, seq, n_embd]
    std::vector<float> residual;        // [batch, seq, n_embd]
    std::vector<float> q;               // [batch, seq, n_head * head_dim]
    std::vector<float> k;               // [batch, seq, n_head_kv * head_dim]
    std::vector<float> v;               // [batch, seq, n_head_kv * head_dim]
    std::vector<float> attn_out;        // [batch, seq, n_head * head_dim]
    std::vector<float> ffn_hidden;      // [batch, seq, n_ff]
    std::vector<float> logits;          // [batch, n_vocab]
    
    // KV cache
    std::vector<float> k_cache;         // [n_layer, n_kv, n_head_kv * head_dim]
    std::vector<float> v_cache;         // [n_layer, n_kv, n_head_kv * head_dim]
    size_t kv_pos;                       // Current position in cache
};

// Main transformer class
class Transformer {
public:
    Transformer();
    ~Transformer();
    
    // Load model from GGUF
    bool load(const std::string& model_path);
    bool load(const GGUFParser& parser);
    
    // Tokenization
    std::vector<int32_t> tokenize(const std::string& text, bool add_bos = true);
    std::string detokenize(const std::vector<int32_t>& tokens);
    std::string detokenize(int32_t token);
    
    // Inference
    bool prefill(const std::vector<int32_t>& tokens);
    bool decode(int32_t token);
    const std::vector<float>& get_logits() const { return state_.logits; }
    
    // Reset state
    void clear_kv_cache();
    
    // Accessors
    const ModelArch& arch() const { return arch_; }
    const TokenizerInfo& tokenizer() const { return tokenizer_; }
    size_t kv_pos() const { return state_.kv_pos; }
    
    // Set RKNN backend (optional, for NPU acceleration)
    void set_backend(std::shared_ptr<RKNNBackend> backend) { backend_ = backend; }
    
private:
    // Forward pass components
    void embed_tokens(const int32_t* tokens, size_t n_tokens);
    void forward_layer(size_t layer_idx, size_t seq_len);
    void forward_attention(size_t layer_idx, size_t seq_len);
    void forward_ffn(size_t layer_idx, size_t seq_len);
    void forward_output();
    
    // Math operations
    void rms_norm(float* out, const float* x, const float* weight, size_t n, float eps);
    void matmul(float* out, const float* a, const GGUFTensor* b, size_t m, size_t k, size_t n);
    void rope(float* q, float* k, size_t n_head, size_t n_head_kv, size_t head_dim, 
              size_t pos, float freq_base, float freq_scale);
    void softmax(float* x, size_t n);
    void silu(float* x, size_t n);
    void gelu(float* x, size_t n);
    
    // Model data
    std::unique_ptr<GGUFParser> parser_;
    ModelArch arch_;
    TokenizerInfo tokenizer_;
    ModelWeights weights_;
    InferenceState state_;
    
    // RKNN backend (optional)
    std::shared_ptr<RKNNBackend> backend_;
    
    bool loaded_;
};

} // namespace rkllm

#endif // TRANSFORMER_H
