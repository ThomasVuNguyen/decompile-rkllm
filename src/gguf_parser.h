/**
 * GGUF Model Format Parser
 * 
 * GGUF (GPT-Generated Unified Format) is the standard format for llama.cpp models.
 * .rkllm files appear to be GGUF files with additional RKNN-specific metadata.
 */

#ifndef GGUF_PARSER_H
#define GGUF_PARSER_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace rkllm {

// GGUF magic and version
constexpr uint32_t GGUF_MAGIC = 0x46554747; // "GGUF"
constexpr uint32_t GGUF_VERSION = 3;

// GGUF data types
enum class GGUFType : uint32_t {
    UINT8 = 0,
    INT8 = 1,
    UINT16 = 2,
    INT16 = 3,
    UINT32 = 4,
    INT32 = 5,
    FLOAT32 = 6,
    BOOL = 7,
    STRING = 8,
    ARRAY = 9,
    UINT64 = 10,
    INT64 = 11,
    FLOAT64 = 12,
};

// GGML tensor types (quantization)
enum class GGMLType : uint32_t {
    F32 = 0,
    F16 = 1,
    Q4_0 = 2,
    Q4_1 = 3,
    Q5_0 = 6,
    Q5_1 = 7,
    Q8_0 = 8,
    Q8_1 = 9,
    Q2_K = 10,
    Q3_K = 11,
    Q4_K = 12,
    Q5_K = 13,
    Q6_K = 14,
    Q8_K = 15,
    IQ2_XXS = 16,
    IQ2_XS = 17,
    IQ3_XXS = 18,
    IQ1_S = 19,
    IQ4_NL = 20,
    IQ3_S = 21,
    IQ2_S = 22,
    IQ4_XS = 23,
    I8 = 24,
    I16 = 25,
    I32 = 26,
    I64 = 27,
    F64 = 28,
    BF16 = 30,
    COUNT
};

// Get bytes per element for a GGML type
inline size_t ggml_type_size(GGMLType type) {
    switch (type) {
        case GGMLType::F32: return 4;
        case GGMLType::F16: return 2;
        case GGMLType::BF16: return 2;
        case GGMLType::Q4_0: return 18;  // block size 32
        case GGMLType::Q4_1: return 20;
        case GGMLType::Q5_0: return 22;
        case GGMLType::Q5_1: return 24;
        case GGMLType::Q8_0: return 34;
        case GGMLType::Q8_1: return 36;
        case GGMLType::Q2_K: return 84;  // block size 256
        case GGMLType::Q3_K: return 110;
        case GGMLType::Q4_K: return 144;
        case GGMLType::Q5_K: return 176;
        case GGMLType::Q6_K: return 210;
        case GGMLType::I8: return 1;
        case GGMLType::I16: return 2;
        case GGMLType::I32: return 4;
        default: return 0;
    }
}

inline size_t ggml_block_size(GGMLType type) {
    switch (type) {
        case GGMLType::F32:
        case GGMLType::F16:
        case GGMLType::BF16:
        case GGMLType::I8:
        case GGMLType::I16:
        case GGMLType::I32:
            return 1;
        case GGMLType::Q4_0:
        case GGMLType::Q4_1:
        case GGMLType::Q5_0:
        case GGMLType::Q5_1:
        case GGMLType::Q8_0:
        case GGMLType::Q8_1:
            return 32;
        case GGMLType::Q2_K:
        case GGMLType::Q3_K:
        case GGMLType::Q4_K:
        case GGMLType::Q5_K:
        case GGMLType::Q6_K:
            return 256;
        default:
            return 1;
    }
}

// GGUF metadata value
struct GGUFValue {
    GGUFType type;
    union {
        uint8_t u8;
        int8_t i8;
        uint16_t u16;
        int16_t i16;
        uint32_t u32;
        int32_t i32;
        uint64_t u64;
        int64_t i64;
        float f32;
        double f64;
        bool b;
    };
    std::string str;
    std::vector<GGUFValue> arr;
    
    // Convenience accessors
    int64_t as_int() const;
    double as_float() const;
    std::string as_string() const;
};

// GGUF tensor info
struct GGUFTensor {
    std::string name;
    uint32_t n_dims;
    uint64_t dims[4];
    GGMLType type;
    uint64_t offset;
    
    // Computed fields
    uint64_t n_elements;
    uint64_t size_bytes;
    void* data;  // Pointer into mmap'd file
};

// Model architecture info
struct ModelArch {
    std::string arch_name;  // llama, qwen, phi, etc.
    int32_t n_vocab;
    int32_t n_ctx_train;
    int32_t n_embd;
    int32_t n_head;
    int32_t n_head_kv;
    int32_t n_layer;
    int32_t n_ff;
    int32_t n_expert;
    int32_t n_expert_used;
    float rope_freq_base;
    float rope_freq_scale;
    int32_t rope_type;
    float norm_eps;
};

// Tokenizer info
struct TokenizerInfo {
    std::string model_type;  // llama, gpt2, etc.
    std::vector<std::string> tokens;
    std::vector<float> scores;
    std::vector<int32_t> token_types;
    int32_t bos_id;
    int32_t eos_id;
    int32_t pad_id;
    int32_t unk_id;
    bool add_bos;
    bool add_eos;
    std::string chat_template;
};

// GGUF file parser
class GGUFParser {
public:
    GGUFParser() = default;
    ~GGUFParser();
    
    // Load GGUF file
    bool load(const std::string& path);
    void unload();
    
    // Access metadata
    bool has_key(const std::string& key) const;
    GGUFValue get_value(const std::string& key) const;
    int64_t get_int(const std::string& key, int64_t default_val = 0) const;
    double get_float(const std::string& key, double default_val = 0.0) const;
    std::string get_string(const std::string& key, const std::string& default_val = "") const;
    
    // Access tensors
    const GGUFTensor* get_tensor(const std::string& name) const;
    std::vector<std::string> tensor_names() const;
    
    // Get parsed model/tokenizer info
    const ModelArch& get_model_arch() const { return model_arch_; }
    const TokenizerInfo& get_tokenizer() const { return tokenizer_; }
    
private:
    bool parse_header();
    bool parse_metadata();
    bool parse_tensors();
    bool read_string(std::string& out);
    bool read_value(GGUFValue& out);
    
    // File mapping
    int fd_ = -1;
    void* mapped_ = nullptr;
    size_t file_size_ = 0;
    const uint8_t* data_ = nullptr;
    size_t pos_ = 0;
    
    // Parsed data
    uint32_t version_ = 0;
    uint64_t n_tensors_ = 0;
    uint64_t n_kv_ = 0;
    uint64_t data_offset_ = 0;
    
    std::map<std::string, GGUFValue> metadata_;
    std::map<std::string, GGUFTensor> tensors_;
    
    ModelArch model_arch_;
    TokenizerInfo tokenizer_;
};

} // namespace rkllm

#endif // GGUF_PARSER_H
