/**
 * RKLLM Test Utility
 * 
 * Simple test program to verify the library works correctly.
 * Usage: ./rkllm_test <model.rkllm> [prompt]
 */

#include "../ghidra1/rkllm.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

// Callback for generated tokens
void llm_callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    if (state == LLM_RUN_NORMAL) {
        printf("%s", result->text);
        fflush(stdout);
    } else if (state == LLM_RUN_FINISH) {
        printf("\n[Generation complete]\n");
    } else if (state == LLM_RUN_ERROR) {
        printf("\n[Error occurred]\n");
    }
}

int main(int argc, char** argv) {
    printf("Open RKLLM Test Utility\n");
    printf("=======================\n\n");
    
    if (argc < 2) {
        printf("Usage: %s <model.rkllm> [prompt]\n", argv[0]);
        printf("\nExample:\n");
        printf("  %s qwen2-1.5b.rkllm \"Hello, how are you?\"\n", argv[0]);
        return 1;
    }
    
    const char* model_path = argv[1];
    const char* prompt = argc > 2 ? argv[2] : "Hello!";
    
    // Create default parameters
    RKLLMParam param = rkllm_createDefaultParam();
    param.model_path = model_path;
    param.max_context_len = 2048;
    param.max_new_tokens = 256;
    param.top_k = 40;
    param.top_p = 0.9f;
    param.temperature = 0.8f;
    
    printf("Loading model: %s\n", model_path);
    
    // Initialize model
    LLMHandle handle = nullptr;
    int ret = rkllm_init(&handle, &param, llm_callback);
    if (ret != 0) {
        printf("Failed to initialize model: %d\n", ret);
        return 1;
    }
    
    // Print timing stats
    printf("\n");
    rkllm_print_timings();
    
    // Create input
    RKLLMInput input;
    memset(&input, 0, sizeof(input));
    input.input_type = RKLLM_INPUT_PROMPT;
    input.prompt_input = prompt;
    
    printf("\nPrompt: %s\n", prompt);
    printf("Response: ");
    fflush(stdout);
    
    // Run inference
    RKLLMInferParam infer_param;
    memset(&infer_param, 0, sizeof(infer_param));
    
    ret = rkllm_run(handle, &input, &infer_param, nullptr);
    if (ret != 0) {
        printf("Inference failed: %d\n", ret);
    }
    
    // Print timing stats
    printf("\n");
    rkllm_print_timings();
    
    // Cleanup
    rkllm_destroy(handle);
    
    printf("\nDone!\n");
    return 0;
}
