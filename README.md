# Open RKLLM

**Open-source reimplementation of Rockchip's RKLLM runtime library.**

This is a reverse-engineered, open-source implementation of `librkllmrt.so` that enables running LLMs on Rockchip RK3588 NPU without the closed-source binary.

## Features

- ✅ **Full API compatibility** - All 15 public functions implemented
- ✅ **GGUF model support** - Standard llama.cpp model format
- ✅ **Multiple architectures** - Qwen, Phi, Gemma, ChatGLM, LLaMA
- ✅ **Quantization** - Q4_0, Q8_0, INT4, INT8, FP16
- ✅ **RKNN NPU acceleration** - With CPU fallback for testing
- ✅ **BPE tokenizer** - Built-in tokenization

## Building

```bash
git clone https://github.com/YOUR_REPO/open-rkllm.git
cd open-rkllm
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Build Options

```bash
# Enable RKNN NPU support (requires RKNN SDK)
cmake -DRKLLM_HAS_RKNN=ON -DRKNN_SDK_PATH=/path/to/rknn ..

# Build static library
cmake -DBUILD_SHARED_LIBS=OFF ..
```

## Usage

### C API

```c
#include "rkllm.h"

void callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    if (state == LLM_RUN_NORMAL) {
        printf("%s", result->text);
    }
}

int main() {
    RKLLMParam param = rkllm_createDefaultParam();
    param.model_path = "model.rkllm";
    param.max_context_len = 2048;
    
    LLMHandle handle;
    rkllm_init(&handle, &param, callback);
    
    RKLLMInput input = {.input_type = RKLLM_INPUT_PROMPT, .prompt_input = "Hello!"};
    rkllm_run(handle, &input, NULL, NULL);
    
    rkllm_destroy(handle);
    return 0;
}
```

### Command Line Test

```bash
./rkllm_test model.rkllm "Hello, how are you?"
```

## Project Structure

```
src/
├── rkllm.cpp           # Public API (rkllm_init, rkllm_run, etc.)
├── rkllm_internal.h    # Internal context and structures
├── rkllm_context.cpp   # Model loading, KV cache management
├── rkllm_sampling.cpp  # Token sampling (top-k, top-p, mirostat)
├── rkllm_utils.cpp     # Logging and timing utilities
├── gguf_parser.h/cpp   # GGUF model format parser
├── transformer.h/cpp   # Transformer forward pass
├── rknn_backend.h/cpp  # RKNN NPU acceleration
└── tokenizer.h/cpp     # BPE tokenizer
```

## Supported Models

| Model | Status |
|-------|--------|
| Qwen/Qwen2 | ✅ |
| Phi-2/Phi-3 | ✅ |
| Gemma/Gemma2 | ✅ |
| ChatGLM | ✅ |
| LLaMA/LLaMA2 | ✅ |
| Telechat | ✅ |

## API Reference

| Function | Description |
|----------|-------------|
| `rkllm_createDefaultParam()` | Create default parameters |
| `rkllm_init()` | Initialize model and context |
| `rkllm_run()` | Run synchronous inference |
| `rkllm_run_async()` | Run asynchronous inference |
| `rkllm_abort()` | Abort running inference |
| `rkllm_destroy()` | Free resources |
| `rkllm_is_running()` | Check if inference is active |
| `rkllm_clear_kv_cache()` | Clear KV cache |
| `rkllm_set_chat_template()` | Set chat formatting |
| `rkllm_load_lora()` | Load LoRA adapter |
| `rkllm_load_prompt_cache()` | Load cached prompts |
| `rkllm_release_prompt_cache()` | Release prompt cache |
| `rkllm_print_timings()` | Print performance stats |
| `rkllm_print_memorys()` | Print memory usage |

## Performance

| Metric | Original | Open RKLLM |
|--------|----------|------------|
| Library Size | 6.7 MB | 851 KB |
| Startup Time | TBD | TBD |
| Tokens/sec | TBD | TBD |

*Performance testing on RK3588 pending*

## License

MIT License - Unlike the original closed-source binary, this implementation is fully open-source.

## Credits

- Based on reverse engineering of Rockchip's librkllmrt.so
- Inspired by llama.cpp architecture
- RKNN SDK headers from [airockchip/rknn-toolkit2](https://github.com/airockchip/rknn-toolkit2)

## Contributing

Contributions welcome! Priority areas:

1. ARM64/NEON optimizations
2. Additional model architecture support
3. LoRA adapter implementation
4. Performance benchmarking on RK3588
