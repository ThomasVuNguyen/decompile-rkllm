/**
 * Open RKLLM - Reverse engineered from librkllmrt.so
 * 
 * This file implements the public RKLLM API functions.
 */

#include "rkllm.h"
#include "rkllm_internal.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <new>

using namespace rkllm;

// Global for timing access (used by rkllm_print_timings which has no handle param)
static TimingStats* g_last_timing = nullptr;

/**
 * Create default parameters
 * Decompiled from: ghidra1/exports/v0.0.1/rkllm_createDefaultParam.c
 */
RKLLMParam rkllm_createDefaultParam() {
    RKLLMParam param;
    memset(&param, 0, sizeof(RKLLMParam));
    
    // Defaults based on decompilation analysis
    param.model_path = nullptr;
    param.max_context_len = 512;        // 0x200
    param.max_new_tokens = -1;          // 0xffffffff = unlimited
    param.top_k = 40;                   // 0x28
    param.n_keep = 0;
    param.top_p = 0.9f;                 // 0x3f666666
    param.temperature = 0.8f;           // 0x3f4ccccd
    param.repeat_penalty = 1.1f;        // 0x3f8ccccd
    param.frequency_penalty = 0.0f;
    param.presence_penalty = 0.0f;
    param.mirostat = 0;
    param.mirostat_tau = 5.0f;          // 0x40a00000
    param.mirostat_eta = 0.1f;          // 0x3dcccccd
    param.skip_special_token = true;
    param.is_async = false;
    param.img_start = "";
    param.img_end = "";
    param.img_content = "";
    
    // Extend params - CPU affinity based on sysconf
    param.extend_param.base_domain_id = 0;
    param.extend_param.embed_flash = 0;
    
    // Check number of CPUs (sysconf(0x54) = _SC_NPROCESSORS_ONLN)
    long n_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (n_cpus >= 8) {
        param.extend_param.enabled_cpus_num = 8;
        param.extend_param.enabled_cpus_mask = 0xF0; // CPUs 4-7 (big cores)
    } else if (n_cpus >= 4) {
        param.extend_param.enabled_cpus_num = 4;
        param.extend_param.enabled_cpus_mask = 0x0F; // CPUs 0-3
    } else {
        param.extend_param.enabled_cpus_num = n_cpus;
        param.extend_param.enabled_cpus_mask = (1 << n_cpus) - 1;
    }
    
    return param;
}

/**
 * Initialize RKLLM context
 * Decompiled from: ghidra1/exports/v0.0.1/rkllm_init.c
 */
int rkllm_init(LLMHandle* handle, RKLLMParam* param, LLMResultCallback callback) {
    if (!handle || !param) {
        rkllm_log("E", "rkllm_init: invalid parameters");
        return -1;
    }
    
    // Allocate context (0x248 = 584 bytes in original)
    RKLLMContextInternal* ctx = new (std::nothrow) RKLLMContextInternal();
    if (!ctx) {
        rkllm_log("E", "rkllm_init: failed to allocate context");
        return -1;
    }
    
    // Copy params
    ctx->params = *param;
    ctx->callback = callback;
    ctx->is_async = param->is_async;
    
    // Load model - FUN_001bbb20 in decompilation
    int ret = rkllm_load_model(ctx, param->model_path);
    if (ret != 0) {
        rkllm_log("E", "rkllm_init: failed to load model: %s", param->model_path);
        delete ctx;
        return ret;
    }
    
    // Initialize KV cache
    ret = rkllm_init_kv_cache(ctx);
    if (ret != 0) {
        rkllm_log("E", "rkllm_init: failed to initialize KV cache");
        delete ctx;
        return ret;
    }
    
    // Initialize RKNN backend
    ret = rkllm_init_rknn_backend(ctx);
    if (ret != 0) {
        rkllm_log("E", "rkllm_init: failed to initialize RKNN backend");
        delete ctx;
        return ret;
    }
    
    // Store global timing pointer
    g_last_timing = &ctx->timing;
    
    *handle = ctx;
    rkllm_log("I", "rkllm_init: success, model loaded");
    return 0;
}

/**
 * Destroy RKLLM context
 * Decompiled from: ghidra1/exports/v0.0.1/rkllm_destroy.c
 */
int rkllm_destroy(LLMHandle handle) {
    if (!handle) {
        return -1;
    }
    
    RKLLMContextInternal* ctx = static_cast<RKLLMContextInternal*>(handle);
    
    // Cleanup RKNN contexts
    for (auto& matmul_ctx : ctx->matmul_contexts) {
        // rknn_matmul_destroy(matmul_ctx);
        (void)matmul_ctx;
    }
    
    // Free KV cache
    if (ctx->kv_cache.k_cache) {
        free(ctx->kv_cache.k_cache);
    }
    if (ctx->kv_cache.v_cache) {
        free(ctx->kv_cache.v_cache);
    }
    
    // Free model data
    if (ctx->model_data) {
        free(ctx->model_data);
    }
    
    // Free embed flash buffer
    if (ctx->embed_flash_buffer) {
        free(ctx->embed_flash_buffer);
    }
    
    delete ctx;
    return 0;
}

/**
 * Run inference (synchronous)
 */
int rkllm_run(LLMHandle handle, RKLLMInput* input, RKLLMInferParam* infer_params, void* userdata) {
    if (!handle || !input) {
        return -1;
    }
    
    RKLLMContextInternal* ctx = static_cast<RKLLMContextInternal*>(handle);
    ctx->userdata = userdata;
    ctx->is_running = true;
    ctx->abort_requested = false;
    
    // TODO: Implement actual inference
    // This is where the main work happens - FUN_001be860 in decompilation
    
    rkllm_log("W", "rkllm_run: not yet implemented");
    
    ctx->is_running = false;
    return 0;
}

/**
 * Run inference (asynchronous)
 */
int rkllm_run_async(LLMHandle handle, RKLLMInput* input, RKLLMInferParam* infer_params, void* userdata) {
    // For now, just call sync version
    // Real implementation would spawn a thread
    return rkllm_run(handle, input, infer_params, userdata);
}

/**
 * Abort running inference
 */
int rkllm_abort(LLMHandle handle) {
    if (!handle) {
        return -1;
    }
    RKLLMContextInternal* ctx = static_cast<RKLLMContextInternal*>(handle);
    ctx->abort_requested = true;
    return 0;
}

/**
 * Check if inference is running
 */
int rkllm_is_running(LLMHandle handle) {
    if (!handle) {
        return 0;
    }
    RKLLMContextInternal* ctx = static_cast<RKLLMContextInternal*>(handle);
    return ctx->is_running ? 0 : 1; // Note: original returns 0 if running
}

/**
 * Clear KV cache
 */
int rkllm_clear_kv_cache(LLMHandle handle, int keep_system_prompt) {
    if (!handle) {
        return -1;
    }
    RKLLMContextInternal* ctx = static_cast<RKLLMContextInternal*>(handle);
    
    if (keep_system_prompt) {
        // Keep first N tokens in cache
        // ctx->kv_cache.n_tokens = ctx->params.n_keep;
    } else {
        ctx->kv_cache.n_tokens = 0;
    }
    
    return 0;
}

/**
 * Set chat template
 */
int rkllm_set_chat_template(LLMHandle handle, const char* system_prompt, 
                            const char* prompt_prefix, const char* prompt_postfix) {
    if (!handle) {
        return -1;
    }
    RKLLMContextInternal* ctx = static_cast<RKLLMContextInternal*>(handle);
    
    if (system_prompt) ctx->chat_template.system_prompt = system_prompt;
    if (prompt_prefix) ctx->chat_template.prompt_prefix = prompt_prefix;
    if (prompt_postfix) ctx->chat_template.prompt_postfix = prompt_postfix;
    
    rkllm_log("I", "rkllm: reset chat template:");
    
    return 0;
}

/**
 * Load LoRA adapter
 */
int rkllm_load_lora(LLMHandle handle, RKLLMLoraAdapter* lora_adapter) {
    if (!handle || !lora_adapter) {
        return -1;
    }
    RKLLMContextInternal* ctx = static_cast<RKLLMContextInternal*>(handle);
    
    ctx->lora_adapters.emplace_back(
        lora_adapter->lora_adapter_path,
        lora_adapter->scale
    );
    
    // TODO: Actually load and merge LoRA weights
    rkllm_log("W", "rkllm_load_lora: not yet implemented");
    
    return 0;
}

/**
 * Load prompt cache
 */
int rkllm_load_prompt_cache(LLMHandle handle, const char* prompt_cache_path) {
    if (!handle || !prompt_cache_path) {
        return -1;
    }
    RKLLMContextInternal* ctx = static_cast<RKLLMContextInternal*>(handle);
    
    // TODO: Load cached KV cache state
    ctx->prompt_cache_path = prompt_cache_path;
    rkllm_log("W", "rkllm_load_prompt_cache: not yet implemented");
    
    return 0;
}

/**
 * Release prompt cache
 */
int rkllm_release_prompt_cache(LLMHandle handle) {
    if (!handle) {
        return -1;
    }
    RKLLMContextInternal* ctx = static_cast<RKLLMContextInternal*>(handle);
    
    ctx->prompt_cache_path.clear();
    return 0;
}
