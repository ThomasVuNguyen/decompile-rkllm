/**
 * BPE Tokenizer Implementation
 * 
 * Byte-Pair Encoding tokenizer for LLM text processing.
 * Compatible with llama.cpp/GGUF tokenizer format.
 */

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <cstdint>

namespace rkllm {

class Tokenizer {
public:
    Tokenizer();
    ~Tokenizer() = default;
    
    // Load from GGUF tokenizer data
    bool load(const std::vector<std::string>& tokens,
              const std::vector<float>& scores,
              const std::vector<int32_t>& token_types);
    
    // Load BPE merges (if available)
    bool load_merges(const std::string& merges_text);
    
    // Configure special tokens
    void set_special_tokens(int32_t bos, int32_t eos, int32_t unk, int32_t pad);
    void set_add_bos(bool add_bos) { add_bos_ = add_bos; }
    void set_add_eos(bool add_eos) { add_eos_ = add_eos; }
    
    // Tokenization
    std::vector<int32_t> encode(const std::string& text, bool add_special = true) const;
    std::string decode(const std::vector<int32_t>& tokens) const;
    std::string decode(int32_t token) const;
    
    // Accessors
    size_t vocab_size() const { return tokens_.size(); }
    int32_t bos_id() const { return bos_id_; }
    int32_t eos_id() const { return eos_id_; }
    int32_t unk_id() const { return unk_id_; }
    int32_t pad_id() const { return pad_id_; }
    
    // Get token string
    const std::string& get_token(int32_t id) const;
    
    // Get token ID (-1 if not found)
    int32_t get_token_id(const std::string& token) const;
    
private:
    // BPE merge step
    void bpe_merge(std::vector<std::string>& pieces) const;
    
    // Apply BPE to a single word
    std::vector<int32_t> bpe_encode(const std::string& word) const;
    
    // Pre-tokenize text into words
    std::vector<std::string> pre_tokenize(const std::string& text) const;
    
    // Vocabulary
    std::vector<std::string> tokens_;
    std::vector<float> scores_;
    std::unordered_map<std::string, int32_t> token_to_id_;
    
    // BPE merges: (pair) -> merged token
    std::unordered_map<std::string, std::string> merges_;
    std::unordered_map<std::string, int32_t> merge_ranks_;
    
    // Special tokens
    int32_t bos_id_ = 1;
    int32_t eos_id_ = 2;
    int32_t unk_id_ = 0;
    int32_t pad_id_ = 0;
    bool add_bos_ = true;
    bool add_eos_ = false;
    
    // Byte-to-unicode mapping (for GPT-2 style tokenizers)
    std::unordered_map<uint8_t, std::string> byte_encoder_;
    std::unordered_map<std::string, uint8_t> byte_decoder_;
};

} // namespace rkllm

#endif // TOKENIZER_H
