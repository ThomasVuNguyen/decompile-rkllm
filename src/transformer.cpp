/**
 * Transformer Model Implementation
 */

#include "transformer.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace rkllm {

// Dequantization helpers
static void dequantize_q4_0(const void* src, float* dst, size_t n);
static void dequantize_q8_0(const void* src, float* dst, size_t n);
static void dequantize_f16(const void* src, float* dst, size_t n);

// Half-precision to float conversion
static float half_to_float(uint16_t h) {
    uint32_t sign = (h >> 15) & 1;
    uint32_t exp = (h >> 10) & 0x1F;
    uint32_t mant = h & 0x3FF;
    
    if (exp == 0) {
        if (mant == 0) return sign ? -0.0f : 0.0f;
        // Denormal
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

void Tensor::dequantize_to(float* dst) const {
    switch (type) {
        case GGMLType::F32:
            memcpy(dst, data, n_elements * sizeof(float));
            break;
        case GGMLType::F16:
            dequantize_f16(data, dst, n_elements);
            break;
        case GGMLType::Q4_0:
            dequantize_q4_0(data, dst, n_elements);
            break;
        case GGMLType::Q8_0:
            dequantize_q8_0(data, dst, n_elements);
            break;
        default:
            rkllm_log("W", "Unsupported tensor type for dequantization: %d", (int)type);
            break;
    }
}

float Tensor::get(size_t idx) const {
    if (type == GGMLType::F32) {
        return static_cast<const float*>(data)[idx];
    } else if (type == GGMLType::F16) {
        return half_to_float(static_cast<const uint16_t*>(data)[idx]);
    }
    // For quantized types, need to compute block offset
    return 0.0f;
}

static void dequantize_f16(const void* src, float* dst, size_t n) {
    const uint16_t* s = static_cast<const uint16_t*>(src);
    for (size_t i = 0; i < n; i++) {
        dst[i] = half_to_float(s[i]);
    }
}

// Q4_0 block structure: 2 bytes scale (fp16) + 16 bytes data (32 x 4-bit)
static void dequantize_q4_0(const void* src, float* dst, size_t n) {
    const size_t block_size = 32;
    const size_t n_blocks = n / block_size;
    const uint8_t* data = static_cast<const uint8_t*>(src);
    
    for (size_t i = 0; i < n_blocks; i++) {
        // Scale is first 2 bytes (FP16)
        uint16_t scale_fp16;
        memcpy(&scale_fp16, data, 2);
        float scale = half_to_float(scale_fp16);
        data += 2;
        
        // 16 bytes of quantized data (32 x 4-bit)
        for (size_t j = 0; j < 16; j++) {
            uint8_t byte = data[j];
            // Low nibble
            int8_t val0 = (byte & 0xF) - 8;
            // High nibble
            int8_t val1 = (byte >> 4) - 8;
            
            dst[i * block_size + j * 2 + 0] = val0 * scale;
            dst[i * block_size + j * 2 + 1] = val1 * scale;
        }
        data += 16;
    }
}

// Q8_0 block structure: 2 bytes scale (fp16) + 32 bytes data (32 x 8-bit)
static void dequantize_q8_0(const void* src, float* dst, size_t n) {
    const size_t block_size = 32;
    const size_t n_blocks = n / block_size;
    const uint8_t* data = static_cast<const uint8_t*>(src);
    
    for (size_t i = 0; i < n_blocks; i++) {
        uint16_t scale_fp16;
        memcpy(&scale_fp16, data, 2);
        float scale = half_to_float(scale_fp16);
        data += 2;
        
        const int8_t* qdata = reinterpret_cast<const int8_t*>(data);
        for (size_t j = 0; j < block_size; j++) {
            dst[i * block_size + j] = qdata[j] * scale;
        }
        data += block_size;
    }
}

Transformer::Transformer() : loaded_(false) {}

Transformer::~Transformer() {}

bool Transformer::load(const std::string& model_path) {
    parser_ = std::make_unique<GGUFParser>();
    if (!parser_->load(model_path)) {
        return false;
    }
    return load(*parser_);
}

bool Transformer::load(const GGUFParser& parser) {
    arch_ = parser.get_model_arch();
    tokenizer_ = parser.get_tokenizer();
    
    // Load base weights
    weights_.token_embd = parser.get_tensor("token_embd.weight");
    weights_.token_embd_norm = parser.get_tensor("token_embd_norm.weight");
    weights_.output_norm = parser.get_tensor("output_norm.weight");
    weights_.output = parser.get_tensor("output.weight");
    
    if (!weights_.token_embd) {
        rkllm_log("E", "Missing token_embd.weight tensor");
        return false;
    }
    
    // Load layer weights
    weights_.layers.resize(arch_.n_layer);
    
    for (int i = 0; i < arch_.n_layer; i++) {
        char name[256];
        auto& layer = weights_.layers[i];
        
        // Attention weights
        snprintf(name, sizeof(name), "blk.%d.attn_q.weight", i);
        layer.attn.q = parser.get_tensor(name);
        
        snprintf(name, sizeof(name), "blk.%d.attn_k.weight", i);
        layer.attn.k = parser.get_tensor(name);
        
        snprintf(name, sizeof(name), "blk.%d.attn_v.weight", i);
        layer.attn.v = parser.get_tensor(name);
        
        snprintf(name, sizeof(name), "blk.%d.attn_output.weight", i);
        layer.attn.o = parser.get_tensor(name);
        
        // Combined QKV (some models like ChatGLM)
        snprintf(name, sizeof(name), "blk.%d.attn_qkv.weight", i);
        layer.attn.qkv = parser.get_tensor(name);
        
        snprintf(name, sizeof(name), "blk.%d.attn_norm.weight", i);
        layer.attn.norm = parser.get_tensor(name);
        
        snprintf(name, sizeof(name), "blk.%d.attn_q_norm.weight", i);
        layer.attn.q_norm = parser.get_tensor(name);
        
        snprintf(name, sizeof(name), "blk.%d.attn_k_norm.weight", i);
        layer.attn.k_norm = parser.get_tensor(name);
        
        // FFN weights
        snprintf(name, sizeof(name), "blk.%d.ffn_gate.weight", i);
        layer.ffn.gate = parser.get_tensor(name);
        
        snprintf(name, sizeof(name), "blk.%d.ffn_up.weight", i);
        layer.ffn.up = parser.get_tensor(name);
        
        snprintf(name, sizeof(name), "blk.%d.ffn_down.weight", i);
        layer.ffn.down = parser.get_tensor(name);
        
        snprintf(name, sizeof(name), "blk.%d.ffn_norm.weight", i);
        layer.ffn.norm = parser.get_tensor(name);
    }
    
    // Allocate state buffers
    size_t n_embd = arch_.n_embd;
    size_t n_ff = arch_.n_ff;
    size_t n_vocab = arch_.n_vocab;
    size_t max_ctx = 4096;  // TODO: Make configurable
    size_t n_head = arch_.n_head;
    size_t n_head_kv = arch_.n_head_kv;
    size_t head_dim = n_embd / n_head;
    
    state_.hidden.resize(n_embd);
    state_.residual.resize(n_embd);
    state_.q.resize(n_head * head_dim);
    state_.k.resize(n_head_kv * head_dim);
    state_.v.resize(n_head_kv * head_dim);
    state_.attn_out.resize(n_embd);
    state_.ffn_hidden.resize(n_ff * 2);  // For gate + up
    state_.logits.resize(n_vocab);
    
    // KV cache
    size_t kv_size = arch_.n_layer * max_ctx * n_head_kv * head_dim;
    state_.k_cache.resize(kv_size, 0.0f);
    state_.v_cache.resize(kv_size, 0.0f);
    state_.kv_pos = 0;
    
    loaded_ = true;
    rkllm_log("I", "Model loaded: %d layers, %zu params", 
              arch_.n_layer, weights_.token_embd->n_elements);
    
    return true;
}

std::vector<int32_t> Transformer::tokenize(const std::string& text, bool add_bos) {
    std::vector<int32_t> tokens;
    
    if (add_bos && tokenizer_.add_bos) {
        tokens.push_back(tokenizer_.bos_id);
    }
    
    // Simple byte-level fallback tokenizer
    // Real implementation would use BPE/SentencePiece
    for (unsigned char c : text) {
        // Try to find exact match in vocab
        bool found = false;
        for (size_t i = 0; i < tokenizer_.tokens.size(); i++) {
            if (tokenizer_.tokens[i].size() == 1 && 
                tokenizer_.tokens[i][0] == static_cast<char>(c)) {
                tokens.push_back(i);
                found = true;
                break;
            }
        }
        if (!found) {
            // Use unknown token
            tokens.push_back(tokenizer_.unk_id);
        }
    }
    
    return tokens;
}

std::string Transformer::detokenize(const std::vector<int32_t>& tokens) {
    std::string result;
    for (int32_t t : tokens) {
        result += detokenize(t);
    }
    return result;
}

std::string Transformer::detokenize(int32_t token) {
    if (token >= 0 && token < (int32_t)tokenizer_.tokens.size()) {
        return tokenizer_.tokens[token];
    }
    return "";
}

void Transformer::clear_kv_cache() {
    std::fill(state_.k_cache.begin(), state_.k_cache.end(), 0.0f);
    std::fill(state_.v_cache.begin(), state_.v_cache.end(), 0.0f);
    state_.kv_pos = 0;
}

void Transformer::embed_tokens(const int32_t* tokens, size_t n_tokens) {
    size_t n_embd = arch_.n_embd;
    
    // For single token, just copy embedding
    if (n_tokens == 1) {
        int32_t token = tokens[0];
        if (weights_.token_embd->type == GGMLType::F32) {
            const float* embd = static_cast<const float*>(weights_.token_embd->data);
            memcpy(state_.hidden.data(), embd + token * n_embd, n_embd * sizeof(float));
        } else if (weights_.token_embd->type == GGMLType::F16) {
            const uint16_t* embd = static_cast<const uint16_t*>(weights_.token_embd->data);
            for (size_t i = 0; i < n_embd; i++) {
                state_.hidden[i] = half_to_float(embd[token * n_embd + i]);
            }
        }
    }
}

void Transformer::rms_norm(float* out, const float* x, const float* weight, size_t n, float eps) {
    // Calculate RMS
    float sum_sq = 0.0f;
    for (size_t i = 0; i < n; i++) {
        sum_sq += x[i] * x[i];
    }
    float rms = sqrtf(sum_sq / n + eps);
    float inv_rms = 1.0f / rms;
    
    // Normalize and scale
    for (size_t i = 0; i < n; i++) {
        out[i] = x[i] * inv_rms * weight[i];
    }
}

void Transformer::matmul(float* out, const float* a, const GGUFTensor* b, 
                         size_t m, size_t k, size_t n) {
    // TODO: Use RKNN backend for acceleration
    // For now, naive CPU implementation with dequantization
    
    std::vector<float> b_f32(k * n);
    
    // Dequantize B
    Tensor t;
    t.data = b->data;
    t.type = b->type;
    t.n_elements = k * n;
    t.dequantize_to(b_f32.data());
    
    // Naive matmul: out[m, n] = a[m, k] @ b[k, n]
    for (size_t i = 0; i < m; i++) {
        for (size_t j = 0; j < n; j++) {
            float sum = 0.0f;
            for (size_t kk = 0; kk < k; kk++) {
                sum += a[i * k + kk] * b_f32[kk * n + j];
            }
            out[i * n + j] = sum;
        }
    }
}

void Transformer::rope(float* q, float* k, size_t n_head, size_t n_head_kv, 
                       size_t head_dim, size_t pos, float freq_base, float freq_scale) {
    // Apply Rotary Position Encoding
    for (size_t h = 0; h < n_head; h++) {
        for (size_t i = 0; i < head_dim / 2; i++) {
            float theta = powf(freq_base, -2.0f * i / head_dim) * pos * freq_scale;
            float cos_theta = cosf(theta);
            float sin_theta = sinf(theta);
            
            size_t idx0 = h * head_dim + i * 2;
            size_t idx1 = h * head_dim + i * 2 + 1;
            
            float q0 = q[idx0];
            float q1 = q[idx1];
            q[idx0] = q0 * cos_theta - q1 * sin_theta;
            q[idx1] = q0 * sin_theta + q1 * cos_theta;
        }
    }
    
    for (size_t h = 0; h < n_head_kv; h++) {
        for (size_t i = 0; i < head_dim / 2; i++) {
            float theta = powf(freq_base, -2.0f * i / head_dim) * pos * freq_scale;
            float cos_theta = cosf(theta);
            float sin_theta = sinf(theta);
            
            size_t idx0 = h * head_dim + i * 2;
            size_t idx1 = h * head_dim + i * 2 + 1;
            
            float k0 = k[idx0];
            float k1 = k[idx1];
            k[idx0] = k0 * cos_theta - k1 * sin_theta;
            k[idx1] = k0 * sin_theta + k1 * cos_theta;
        }
    }
}

void Transformer::softmax(float* x, size_t n) {
    float max_val = x[0];
    for (size_t i = 1; i < n; i++) {
        max_val = std::max(max_val, x[i]);
    }
    
    float sum = 0.0f;
    for (size_t i = 0; i < n; i++) {
        x[i] = expf(x[i] - max_val);
        sum += x[i];
    }
    
    for (size_t i = 0; i < n; i++) {
        x[i] /= sum;
    }
}

void Transformer::silu(float* x, size_t n) {
    for (size_t i = 0; i < n; i++) {
        x[i] = x[i] / (1.0f + expf(-x[i]));
    }
}

void Transformer::gelu(float* x, size_t n) {
    for (size_t i = 0; i < n; i++) {
        x[i] = 0.5f * x[i] * (1.0f + tanhf(0.7978845608f * (x[i] + 0.044715f * x[i] * x[i] * x[i])));
    }
}

void Transformer::forward_attention(size_t layer_idx, size_t seq_len) {
    auto& layer = weights_.layers[layer_idx];
    size_t n_embd = arch_.n_embd;
    size_t n_head = arch_.n_head;
    size_t n_head_kv = arch_.n_head_kv;
    size_t head_dim = n_embd / n_head;
    size_t kv_dim = n_head_kv * head_dim;
    
    // Q, K, V projections
    if (layer.attn.q && layer.attn.k && layer.attn.v) {
        matmul(state_.q.data(), state_.hidden.data(), layer.attn.q, 1, n_embd, n_embd);
        matmul(state_.k.data(), state_.hidden.data(), layer.attn.k, 1, n_embd, kv_dim);
        matmul(state_.v.data(), state_.hidden.data(), layer.attn.v, 1, n_embd, kv_dim);
    }
    
    // Apply RoPE
    rope(state_.q.data(), state_.k.data(), n_head, n_head_kv, head_dim,
         state_.kv_pos, arch_.rope_freq_base, arch_.rope_freq_scale);
    
    // Store K, V in cache
    size_t kv_offset = layer_idx * 4096 * kv_dim + state_.kv_pos * kv_dim;
    memcpy(state_.k_cache.data() + kv_offset, state_.k.data(), kv_dim * sizeof(float));
    memcpy(state_.v_cache.data() + kv_offset, state_.v.data(), kv_dim * sizeof(float));
    
    // Grouped-Query Attention
    size_t n_kv = state_.kv_pos + 1;
    float scale = 1.0f / sqrtf(head_dim);
    
    std::fill(state_.attn_out.begin(), state_.attn_out.end(), 0.0f);
    
    for (size_t h = 0; h < n_head; h++) {
        size_t kv_head = h / (n_head / n_head_kv);
        
        // Compute attention scores for this head
        std::vector<float> scores(n_kv);
        
        for (size_t t = 0; t < n_kv; t++) {
            float score = 0.0f;
            for (size_t d = 0; d < head_dim; d++) {
                float q_val = state_.q[h * head_dim + d];
                float k_val = state_.k_cache[layer_idx * 4096 * kv_dim + t * kv_dim + kv_head * head_dim + d];
                score += q_val * k_val;
            }
            scores[t] = score * scale;
        }
        
        // Softmax
        softmax(scores.data(), n_kv);
        
        // Apply attention to V
        for (size_t d = 0; d < head_dim; d++) {
            float sum = 0.0f;
            for (size_t t = 0; t < n_kv; t++) {
                float v_val = state_.v_cache[layer_idx * 4096 * kv_dim + t * kv_dim + kv_head * head_dim + d];
                sum += scores[t] * v_val;
            }
            state_.attn_out[h * head_dim + d] = sum;
        }
    }
    
    // Output projection
    if (layer.attn.o) {
        std::vector<float> tmp(n_embd);
        matmul(tmp.data(), state_.attn_out.data(), layer.attn.o, 1, n_embd, n_embd);
        memcpy(state_.attn_out.data(), tmp.data(), n_embd * sizeof(float));
    }
}

void Transformer::forward_ffn(size_t layer_idx, size_t seq_len) {
    auto& layer = weights_.layers[layer_idx];
    size_t n_embd = arch_.n_embd;
    size_t n_ff = arch_.n_ff;
    
    // Gate and Up projections
    std::vector<float> gate(n_ff), up(n_ff);
    
    if (layer.ffn.gate) {
        matmul(gate.data(), state_.hidden.data(), layer.ffn.gate, 1, n_embd, n_ff);
    }
    if (layer.ffn.up) {
        matmul(up.data(), state_.hidden.data(), layer.ffn.up, 1, n_embd, n_ff);
    }
    
    // SiLU activation on gate, then multiply with up
    silu(gate.data(), n_ff);
    for (size_t i = 0; i < n_ff; i++) {
        gate[i] *= up[i];
    }
    
    // Down projection
    if (layer.ffn.down) {
        matmul(state_.hidden.data(), gate.data(), layer.ffn.down, 1, n_ff, n_embd);
    }
}

void Transformer::forward_layer(size_t layer_idx, size_t seq_len) {
    auto& layer = weights_.layers[layer_idx];
    size_t n_embd = arch_.n_embd;
    
    // Save residual
    memcpy(state_.residual.data(), state_.hidden.data(), n_embd * sizeof(float));
    
    // Attention norm
    if (layer.attn.norm) {
        std::vector<float> norm_weight(n_embd);
        Tensor t;
        t.data = layer.attn.norm->data;
        t.type = layer.attn.norm->type;
        t.n_elements = n_embd;
        t.dequantize_to(norm_weight.data());
        
        rms_norm(state_.hidden.data(), state_.hidden.data(), norm_weight.data(), 
                 n_embd, arch_.norm_eps);
    }
    
    // Attention
    forward_attention(layer_idx, seq_len);
    
    // Add residual
    for (size_t i = 0; i < n_embd; i++) {
        state_.hidden[i] = state_.residual[i] + state_.attn_out[i];
    }
    
    // FFN residual
    memcpy(state_.residual.data(), state_.hidden.data(), n_embd * sizeof(float));
    
    // FFN norm
    if (layer.ffn.norm) {
        std::vector<float> norm_weight(n_embd);
        Tensor t;
        t.data = layer.ffn.norm->data;
        t.type = layer.ffn.norm->type;
        t.n_elements = n_embd;
        t.dequantize_to(norm_weight.data());
        
        rms_norm(state_.hidden.data(), state_.hidden.data(), norm_weight.data(),
                 n_embd, arch_.norm_eps);
    }
    
    // FFN
    forward_ffn(layer_idx, seq_len);
    
    // Add residual
    for (size_t i = 0; i < n_embd; i++) {
        state_.hidden[i] = state_.residual[i] + state_.hidden[i];
    }
}

void Transformer::forward_output() {
    size_t n_embd = arch_.n_embd;
    size_t n_vocab = arch_.n_vocab;
    
    // Output norm
    if (weights_.output_norm) {
        std::vector<float> norm_weight(n_embd);
        Tensor t;
        t.data = weights_.output_norm->data;
        t.type = weights_.output_norm->type;
        t.n_elements = n_embd;
        t.dequantize_to(norm_weight.data());
        
        rms_norm(state_.hidden.data(), state_.hidden.data(), norm_weight.data(),
                 n_embd, arch_.norm_eps);
    }
    
    // LM head
    const GGUFTensor* lm_head = weights_.output ? weights_.output : weights_.token_embd;
    if (lm_head) {
        matmul(state_.logits.data(), state_.hidden.data(), lm_head, 1, n_embd, n_vocab);
    }
}

bool Transformer::prefill(const std::vector<int32_t>& tokens) {
    if (!loaded_) return false;
    
    // Process tokens one at a time (could be batched for efficiency)
    for (size_t i = 0; i < tokens.size(); i++) {
        embed_tokens(&tokens[i], 1);
        
        for (int l = 0; l < arch_.n_layer; l++) {
            forward_layer(l, 1);
        }
        
        state_.kv_pos++;
    }
    
    forward_output();
    return true;
}

bool Transformer::decode(int32_t token) {
    if (!loaded_) return false;
    
    embed_tokens(&token, 1);
    
    for (int l = 0; l < arch_.n_layer; l++) {
        forward_layer(l, 1);
    }
    
    state_.kv_pos++;
    forward_output();
    
    return true;
}

} // namespace rkllm
