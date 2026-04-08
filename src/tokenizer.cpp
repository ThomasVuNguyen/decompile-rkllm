/**
 * BPE Tokenizer Implementation
 */

#include "tokenizer.h"
#include "rkllm_internal.h"
#include <algorithm>
#include <sstream>
#include <climits>

namespace rkllm {

Tokenizer::Tokenizer() {
    // Initialize byte encoder (GPT-2 style)
    // Maps bytes 0-255 to unicode characters
    int n = 0;
    for (int i = 33; i <= 126; i++) {
        byte_encoder_[i] = std::string(1, (char)i);
        byte_decoder_[std::string(1, (char)i)] = i;
        n++;
    }
    for (int i = 161; i <= 172; i++) {
        byte_encoder_[i] = std::string(1, (char)i);
        byte_decoder_[std::string(1, (char)i)] = i;
        n++;
    }
    for (int i = 174; i <= 255; i++) {
        byte_encoder_[i] = std::string(1, (char)i);
        byte_decoder_[std::string(1, (char)i)] = i;
        n++;
    }
    
    // Fill remaining with special characters
    int special = 256;
    for (int i = 0; i < 256; i++) {
        if (byte_encoder_.find(i) == byte_encoder_.end()) {
            // Use unicode codepoint that won't conflict
            byte_encoder_[i] = std::string(1, (char)(special++));
        }
    }
}

bool Tokenizer::load(const std::vector<std::string>& tokens,
                     const std::vector<float>& scores,
                     const std::vector<int32_t>& token_types) {
    tokens_ = tokens;
    scores_ = scores;
    
    token_to_id_.clear();
    for (size_t i = 0; i < tokens_.size(); i++) {
        token_to_id_[tokens_[i]] = i;
    }
    
    rkllm_log("I", "Loaded tokenizer with %zu tokens", tokens_.size());
    return true;
}

bool Tokenizer::load_merges(const std::string& merges_text) {
    std::istringstream stream(merges_text);
    std::string line;
    int rank = 0;
    
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        size_t space = line.find(' ');
        if (space == std::string::npos) continue;
        
        std::string left = line.substr(0, space);
        std::string right = line.substr(space + 1);
        std::string merged = left + right;
        std::string pair_key = left + " " + right;
        
        merges_[pair_key] = merged;
        merge_ranks_[pair_key] = rank++;
    }
    
    rkllm_log("I", "Loaded %zu BPE merges", merges_.size());
    return true;
}

void Tokenizer::set_special_tokens(int32_t bos, int32_t eos, int32_t unk, int32_t pad) {
    bos_id_ = bos;
    eos_id_ = eos;
    unk_id_ = unk;
    pad_id_ = pad;
}

const std::string& Tokenizer::get_token(int32_t id) const {
    static const std::string empty;
    if (id >= 0 && id < (int32_t)tokens_.size()) {
        return tokens_[id];
    }
    return empty;
}

int32_t Tokenizer::get_token_id(const std::string& token) const {
    auto it = token_to_id_.find(token);
    if (it != token_to_id_.end()) {
        return it->second;
    }
    return -1;
}

std::vector<std::string> Tokenizer::pre_tokenize(const std::string& text) const {
    std::vector<std::string> words;
    
    // Simple whitespace-based pre-tokenization
    // Real implementation would use regex for GPT-2 pattern
    std::string current;
    for (char c : text) {
        if (c == ' ' || c == '\n' || c == '\t') {
            if (!current.empty()) {
                words.push_back(current);
                current.clear();
            }
            // Include whitespace as separate token
            words.push_back(std::string(1, c));
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        words.push_back(current);
    }
    
    return words;
}

void Tokenizer::bpe_merge(std::vector<std::string>& pieces) const {
    while (pieces.size() > 1) {
        // Find best merge pair
        int best_idx = -1;
        int best_rank = INT_MAX;
        
        for (size_t i = 0; i < pieces.size() - 1; i++) {
            std::string pair_key = pieces[i] + " " + pieces[i + 1];
            auto it = merge_ranks_.find(pair_key);
            if (it != merge_ranks_.end() && it->second < best_rank) {
                best_rank = it->second;
                best_idx = i;
            }
        }
        
        if (best_idx < 0) break;  // No more merges possible
        
        // Apply merge
        std::string pair_key = pieces[best_idx] + " " + pieces[best_idx + 1];
        pieces[best_idx] = merges_.at(pair_key);
        pieces.erase(pieces.begin() + best_idx + 1);
    }
}

std::vector<int32_t> Tokenizer::bpe_encode(const std::string& word) const {
    std::vector<int32_t> result;
    
    if (word.empty()) return result;
    
    // Try exact match first
    auto it = token_to_id_.find(word);
    if (it != token_to_id_.end()) {
        result.push_back(it->second);
        return result;
    }
    
    // Split into characters and apply BPE
    std::vector<std::string> pieces;
    for (unsigned char c : word) {
        pieces.push_back(std::string(1, c));
    }
    
    // Apply BPE merges
    bpe_merge(pieces);
    
    // Convert pieces to token IDs
    for (const auto& piece : pieces) {
        auto it = token_to_id_.find(piece);
        if (it != token_to_id_.end()) {
            result.push_back(it->second);
        } else {
            // Fall back to byte-level encoding or UNK
            for (unsigned char c : piece) {
                std::string byte_str(1, c);
                auto byte_it = token_to_id_.find(byte_str);
                if (byte_it != token_to_id_.end()) {
                    result.push_back(byte_it->second);
                } else {
                    result.push_back(unk_id_);
                }
            }
        }
    }
    
    return result;
}

std::vector<int32_t> Tokenizer::encode(const std::string& text, bool add_special) const {
    std::vector<int32_t> result;
    
    // Add BOS if configured
    if (add_special && add_bos_) {
        result.push_back(bos_id_);
    }
    
    // Pre-tokenize
    std::vector<std::string> words = pre_tokenize(text);
    
    // Encode each word
    for (const auto& word : words) {
        std::vector<int32_t> ids = bpe_encode(word);
        result.insert(result.end(), ids.begin(), ids.end());
    }
    
    // Add EOS if configured
    if (add_special && add_eos_) {
        result.push_back(eos_id_);
    }
    
    return result;
}

std::string Tokenizer::decode(const std::vector<int32_t>& tokens) const {
    std::string result;
    for (int32_t id : tokens) {
        result += decode(id);
    }
    return result;
}

std::string Tokenizer::decode(int32_t token) const {
    if (token >= 0 && token < (int32_t)tokens_.size()) {
        const std::string& s = tokens_[token];
        
        // Handle special replacement characters (like Ġ for space)
        std::string result;
        for (size_t i = 0; i < s.size(); i++) {
            // Check for Ġ (U+0120) - represents space in GPT-2
            if ((unsigned char)s[i] == 0xC4 && i + 1 < s.size() && 
                (unsigned char)s[i + 1] == 0xA0) {
                result += ' ';
                i++;
            }
            // Check for Ċ (U+010A) - represents newline  
            else if ((unsigned char)s[i] == 0xC4 && i + 1 < s.size() &&
                     (unsigned char)s[i + 1] == 0x8A) {
                result += '\n';
                i++;
            }
            else {
                result += s[i];
            }
        }
        return result;
    }
    return "";
}

} // namespace rkllm
