/**
 * GGUF Model Format Parser Implementation
 */

#include "gguf_parser.h"
#include "rkllm_internal.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <cstring>
#include <cmath>

namespace rkllm {

// GGUFValue implementations
int64_t GGUFValue::as_int() const {
    switch (type) {
        case GGUFType::UINT8: return u8;
        case GGUFType::INT8: return i8;
        case GGUFType::UINT16: return u16;
        case GGUFType::INT16: return i16;
        case GGUFType::UINT32: return u32;
        case GGUFType::INT32: return i32;
        case GGUFType::UINT64: return (int64_t)u64;
        case GGUFType::INT64: return i64;
        case GGUFType::BOOL: return b ? 1 : 0;
        default: return 0;
    }
}

double GGUFValue::as_float() const {
    switch (type) {
        case GGUFType::FLOAT32: return f32;
        case GGUFType::FLOAT64: return f64;
        default: return (double)as_int();
    }
}

std::string GGUFValue::as_string() const {
    if (type == GGUFType::STRING) return str;
    return "";
}

// Parser implementation
GGUFParser::~GGUFParser() {
    unload();
}

bool GGUFParser::load(const std::string& path) {
    unload();
    
    // Open file
    fd_ = open(path.c_str(), O_RDONLY);
    if (fd_ < 0) {
        rkllm_log("E", "Failed to open file: %s", path.c_str());
        return false;
    }
    
    // Get file size
    struct stat st;
    if (fstat(fd_, &st) < 0) {
        close(fd_);
        fd_ = -1;
        return false;
    }
    file_size_ = st.st_size;
    
    // Memory map the file
    mapped_ = mmap(nullptr, file_size_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (mapped_ == MAP_FAILED) {
        rkllm_log("E", "Failed to mmap file: %s", path.c_str());
        close(fd_);
        fd_ = -1;
        return false;
    }
    
    data_ = static_cast<const uint8_t*>(mapped_);
    pos_ = 0;
    
    // Parse GGUF structure
    if (!parse_header()) {
        unload();
        return false;
    }
    
    if (!parse_metadata()) {
        unload();
        return false;
    }
    
    if (!parse_tensors()) {
        unload();
        return false;
    }
    
    rkllm_log("I", "Loaded GGUF: %zu tensors, %zu metadata keys", 
              tensors_.size(), metadata_.size());
    
    return true;
}

void GGUFParser::unload() {
    if (mapped_ && mapped_ != MAP_FAILED) {
        munmap(mapped_, file_size_);
        mapped_ = nullptr;
    }
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
    data_ = nullptr;
    pos_ = 0;
    file_size_ = 0;
    metadata_.clear();
    tensors_.clear();
}

bool GGUFParser::parse_header() {
    if (pos_ + 24 > file_size_) return false;
    
    // Read magic
    uint32_t magic;
    memcpy(&magic, data_ + pos_, 4);
    pos_ += 4;
    
    if (magic != GGUF_MAGIC) {
        rkllm_log("E", "Invalid GGUF magic: 0x%08x", magic);
        return false;
    }
    
    // Read version
    memcpy(&version_, data_ + pos_, 4);
    pos_ += 4;
    
    if (version_ < 2 || version_ > 3) {
        rkllm_log("E", "Unsupported GGUF version: %u", version_);
        return false;
    }
    
    // Read tensor count and KV count
    memcpy(&n_tensors_, data_ + pos_, 8);
    pos_ += 8;
    memcpy(&n_kv_, data_ + pos_, 8);
    pos_ += 8;
    
    rkllm_log("I", "GGUF v%u: %lu tensors, %lu metadata entries", 
              version_, n_tensors_, n_kv_);
    
    return true;
}

bool GGUFParser::read_string(std::string& out) {
    if (pos_ + 8 > file_size_) return false;
    
    uint64_t len;
    memcpy(&len, data_ + pos_, 8);
    pos_ += 8;
    
    if (pos_ + len > file_size_) return false;
    
    out.assign(reinterpret_cast<const char*>(data_ + pos_), len);
    pos_ += len;
    
    return true;
}

bool GGUFParser::read_value(GGUFValue& out) {
    if (pos_ + 4 > file_size_) return false;
    
    uint32_t type_raw;
    memcpy(&type_raw, data_ + pos_, 4);
    pos_ += 4;
    out.type = static_cast<GGUFType>(type_raw);
    
    switch (out.type) {
        case GGUFType::UINT8:
            memcpy(&out.u8, data_ + pos_, 1);
            pos_ += 1;
            break;
        case GGUFType::INT8:
            memcpy(&out.i8, data_ + pos_, 1);
            pos_ += 1;
            break;
        case GGUFType::UINT16:
            memcpy(&out.u16, data_ + pos_, 2);
            pos_ += 2;
            break;
        case GGUFType::INT16:
            memcpy(&out.i16, data_ + pos_, 2);
            pos_ += 2;
            break;
        case GGUFType::UINT32:
            memcpy(&out.u32, data_ + pos_, 4);
            pos_ += 4;
            break;
        case GGUFType::INT32:
            memcpy(&out.i32, data_ + pos_, 4);
            pos_ += 4;
            break;
        case GGUFType::UINT64:
            memcpy(&out.u64, data_ + pos_, 8);
            pos_ += 8;
            break;
        case GGUFType::INT64:
            memcpy(&out.i64, data_ + pos_, 8);
            pos_ += 8;
            break;
        case GGUFType::FLOAT32:
            memcpy(&out.f32, data_ + pos_, 4);
            pos_ += 4;
            break;
        case GGUFType::FLOAT64:
            memcpy(&out.f64, data_ + pos_, 8);
            pos_ += 8;
            break;
        case GGUFType::BOOL:
            out.b = (data_[pos_] != 0);
            pos_ += 1;
            break;
        case GGUFType::STRING:
            if (!read_string(out.str)) return false;
            break;
        case GGUFType::ARRAY: {
            uint32_t arr_type;
            uint64_t arr_len;
            memcpy(&arr_type, data_ + pos_, 4);
            pos_ += 4;
            memcpy(&arr_len, data_ + pos_, 8);
            pos_ += 8;
            
            out.arr.resize(arr_len);
            for (uint64_t i = 0; i < arr_len; i++) {
                out.arr[i].type = static_cast<GGUFType>(arr_type);
                switch (static_cast<GGUFType>(arr_type)) {
                    case GGUFType::STRING:
                        if (!read_string(out.arr[i].str)) return false;
                        break;
                    case GGUFType::FLOAT32:
                        memcpy(&out.arr[i].f32, data_ + pos_, 4);
                        pos_ += 4;
                        break;
                    case GGUFType::INT32:
                        memcpy(&out.arr[i].i32, data_ + pos_, 4);
                        pos_ += 4;
                        break;
                    default:
                        // Skip other array types for now
                        break;
                }
            }
            break;
        }
        default:
            rkllm_log("W", "Unknown GGUF value type: %u", type_raw);
            return false;
    }
    
    return true;
}

bool GGUFParser::parse_metadata() {
    for (uint64_t i = 0; i < n_kv_; i++) {
        std::string key;
        if (!read_string(key)) return false;
        
        GGUFValue value;
        if (!read_value(value)) return false;
        
        metadata_[key] = std::move(value);
    }
    
    // Extract model architecture
    model_arch_.arch_name = get_string("general.architecture", "llama");
    std::string arch = model_arch_.arch_name;
    
    model_arch_.n_vocab = get_int(arch + ".vocab_size", 32000);
    model_arch_.n_ctx_train = get_int(arch + ".context_length", 2048);
    model_arch_.n_embd = get_int(arch + ".embedding_length", 4096);
    model_arch_.n_head = get_int(arch + ".attention.head_count", 32);
    model_arch_.n_head_kv = get_int(arch + ".attention.head_count_kv", model_arch_.n_head);
    model_arch_.n_layer = get_int(arch + ".block_count", 32);
    model_arch_.n_ff = get_int(arch + ".feed_forward_length", model_arch_.n_embd * 4);
    model_arch_.n_expert = get_int(arch + ".expert_count", 0);
    model_arch_.n_expert_used = get_int(arch + ".expert_used_count", 0);
    model_arch_.rope_freq_base = get_float(arch + ".rope.freq_base", 10000.0);
    model_arch_.rope_freq_scale = get_float("rope_scaling.factor", 1.0);
    model_arch_.rope_type = get_int(arch + ".rope.scaling.type", 0);
    model_arch_.norm_eps = get_float(arch + ".attention.layer_norm_rms_epsilon", 1e-5);
    
    rkllm_log("I", "Model: %s, %d layers, %d embd, %d heads", 
              arch.c_str(), model_arch_.n_layer, model_arch_.n_embd, model_arch_.n_head);
    
    // Extract tokenizer info
    tokenizer_.bos_id = get_int("tokenizer.ggml.bos_token_id", 1);
    tokenizer_.eos_id = get_int("tokenizer.ggml.eos_token_id", 2);
    tokenizer_.pad_id = get_int("tokenizer.ggml.padding_token_id", 0);
    tokenizer_.unk_id = get_int("tokenizer.ggml.unknown_token_id", 0);
    tokenizer_.add_bos = get_int("tokenizer.ggml.add_bos_token", 1) != 0;
    tokenizer_.add_eos = get_int("tokenizer.ggml.add_eos_token", 0) != 0;
    tokenizer_.chat_template = get_string("tokenizer.chat_template", "");
    
    // Extract token list
    if (has_key("tokenizer.ggml.tokens")) {
        const auto& tokens_val = get_value("tokenizer.ggml.tokens");
        if (tokens_val.type == GGUFType::ARRAY) {
            for (const auto& t : tokens_val.arr) {
                tokenizer_.tokens.push_back(t.str);
            }
        }
    }
    
    rkllm_log("I", "Tokenizer: vocab_size=%zu, bos=%d, eos=%d", 
              tokenizer_.tokens.size(), tokenizer_.bos_id, tokenizer_.eos_id);
    
    return true;
}

bool GGUFParser::parse_tensors() {
    // Align to 32 bytes for data section
    data_offset_ = pos_;
    
    // Read tensor info entries
    for (uint64_t i = 0; i < n_tensors_; i++) {
        GGUFTensor tensor;
        
        if (!read_string(tensor.name)) return false;
        
        memcpy(&tensor.n_dims, data_ + pos_, 4);
        pos_ += 4;
        
        tensor.n_elements = 1;
        for (uint32_t d = 0; d < tensor.n_dims; d++) {
            memcpy(&tensor.dims[d], data_ + pos_, 8);
            pos_ += 8;
            tensor.n_elements *= tensor.dims[d];
        }
        
        uint32_t type_raw;
        memcpy(&type_raw, data_ + pos_, 4);
        pos_ += 4;
        tensor.type = static_cast<GGMLType>(type_raw);
        
        memcpy(&tensor.offset, data_ + pos_, 8);
        pos_ += 8;
        
        // Calculate size
        size_t block_size = ggml_block_size(tensor.type);
        size_t type_size = ggml_type_size(tensor.type);
        tensor.size_bytes = (tensor.n_elements / block_size) * type_size;
        
        tensors_[tensor.name] = tensor;
    }
    
    // Calculate data section start (align to 32)
    data_offset_ = (pos_ + 31) & ~31ULL;
    
    // Set data pointers for each tensor
    for (auto& kv : tensors_) {
        kv.second.data = const_cast<void*>(
            static_cast<const void*>(data_ + data_offset_ + kv.second.offset));
    }
    
    return true;
}

bool GGUFParser::has_key(const std::string& key) const {
    return metadata_.find(key) != metadata_.end();
}

GGUFValue GGUFParser::get_value(const std::string& key) const {
    auto it = metadata_.find(key);
    if (it != metadata_.end()) {
        return it->second;
    }
    return GGUFValue{};
}

int64_t GGUFParser::get_int(const std::string& key, int64_t default_val) const {
    auto it = metadata_.find(key);
    if (it != metadata_.end()) {
        return it->second.as_int();
    }
    return default_val;
}

double GGUFParser::get_float(const std::string& key, double default_val) const {
    auto it = metadata_.find(key);
    if (it != metadata_.end()) {
        return it->second.as_float();
    }
    return default_val;
}

std::string GGUFParser::get_string(const std::string& key, const std::string& default_val) const {
    auto it = metadata_.find(key);
    if (it != metadata_.end()) {
        return it->second.as_string();
    }
    return default_val;
}

const GGUFTensor* GGUFParser::get_tensor(const std::string& name) const {
    auto it = tensors_.find(name);
    if (it != tensors_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> GGUFParser::tensor_names() const {
    std::vector<std::string> names;
    for (const auto& kv : tensors_) {
        names.push_back(kv.first);
    }
    return names;
}

} // namespace rkllm
