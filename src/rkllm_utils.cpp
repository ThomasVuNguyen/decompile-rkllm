/**
 * RKLLM Utilities
 * 
 * Logging, timing, memory stats, etc.
 * Decompiled from: ghidra1/exports/v0.0.1/rkllm_print_timings.c
 *                  ghidra1/exports/v0.0.1/rkllm_print_memorys.c
 */

#include "rkllm_internal.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>

namespace rkllm {

// Global timing stats for print functions
static TimingStats g_timing_stats;

/**
 * Logging function
 * Mimics the original "I rkllm:", "W rkllm:", "E rkllm:" format
 */
void rkllm_log(const char* level, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    fprintf(stdout, "%s rkllm: ", level);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
    fflush(stdout);
    
    va_end(args);
}

} // namespace rkllm

/**
 * Print timing statistics
 * This is an exported function with no handle parameter
 * 
 * Decompiled format:
 * I rkllm: --------------------------------------------------------------------------------------
 * I rkllm:  Stage         Total Time (ms)  Tokens    Time per Token (ms)      Tokens per Second    
 * I rkllm: --------------------------------------------------------------------------------------
 * I rkllm:  Model Load    1234.56          -         -                        -
 * I rkllm:  Prefill       567.89           128       4.44                     225.11
 * I rkllm:  Generate      3456.78          256       13.50                    74.07
 * I rkllm: --------------------------------------------------------------------------------------
 */
extern "C" {

int rkllm_print_timings(void) {
    extern rkllm::TimingStats* g_last_timing;  // From rkllm.cpp
    
    const char* line = "I rkllm: --------------------------------------------------------------------------------------";
    
    fputs(line, stdout);
    fputc('\n', stdout);
    fflush(stdout);
    
    fprintf(stdout, "I rkllm:  %-12s  %-15s  %-8s  %-23s  %-23s",
            "Stage", "Total Time (ms)", "Tokens", "Time per Token (ms)", "Tokens per Second");
    fputc('\n', stdout);
    fflush(stdout);
    
    fputs(line, stdout);
    fputc('\n', stdout);
    fflush(stdout);
    
    if (g_last_timing) {
        // Model load
        fprintf(stdout, "I rkllm:  %-12s  %-15.2f  %-8s  %-23s  %-23s",
                "Model Load", g_last_timing->load_time_ms, "-", "-", "-");
        fputc('\n', stdout);
        fflush(stdout);
        
        // Prefill
        double prefill_ms_per_token = 0.0;
        double prefill_toks_per_sec = 0.0;
        if (g_last_timing->prefill_tokens > 0) {
            prefill_ms_per_token = g_last_timing->prefill_time_ms / g_last_timing->prefill_tokens;
            prefill_toks_per_sec = 1000.0 / g_last_timing->prefill_time_ms * g_last_timing->prefill_tokens;
        }
        fprintf(stdout, "I rkllm:  %-12s  %-15.2f  %-8d  %-23.2f  %-23.2f",
                "Prefill", g_last_timing->prefill_time_ms, g_last_timing->prefill_tokens,
                prefill_ms_per_token, prefill_toks_per_sec);
        fputc('\n', stdout);
        fflush(stdout);
        
        // Generate
        double generate_total = g_last_timing->generate_time_ms + g_last_timing->prefill_extra_ms;
        double gen_ms_per_token = 0.0;
        double gen_toks_per_sec = 0.0;
        if (g_last_timing->generate_tokens > 0) {
            gen_ms_per_token = generate_total / g_last_timing->generate_tokens;
            gen_toks_per_sec = 1000.0 / generate_total * g_last_timing->generate_tokens;
        }
        fprintf(stdout, "I rkllm:  %-12s  %-15.2f  %-8d  %-23.2f  %-23.2f",
                "Generate", generate_total, g_last_timing->generate_tokens,
                gen_ms_per_token, gen_toks_per_sec);
        fputc('\n', stdout);
        fflush(stdout);
    }
    
    fputs(line, stdout);
    fputc('\n', stdout);
    return fflush(stdout);
}

/**
 * Print memory statistics
 * Decompiled from: ghidra1/exports/v0.0.1/rkllm_print_memorys.c
 */
int rkllm_print_memorys(void) {
    const char* line = "I rkllm: --------------------------------------------------------------------------------------";
    
    fputs(line, stdout);
    fputc('\n', stdout);
    
    fprintf(stdout, "I rkllm:  %-20s  %-20s\n", "Component", "Memory (MB)");
    
    fputs(line, stdout);
    fputc('\n', stdout);
    
    // TODO: Track actual memory usage
    fprintf(stdout, "I rkllm:  %-20s  %-20.2f\n", "Model Weights", 0.0);
    fprintf(stdout, "I rkllm:  %-20s  %-20.2f\n", "KV Cache", 0.0);
    fprintf(stdout, "I rkllm:  %-20s  %-20.2f\n", "Activations", 0.0);
    fprintf(stdout, "I rkllm:  %-20s  %-20.2f\n", "Total", 0.0);
    
    fputs(line, stdout);
    fputc('\n', stdout);
    
    return fflush(stdout);
}

/**
 * Accuracy analysis function  
 * This is used for debugging quantization quality
 * Decompiled from: ghidra1/exports/v0.0.1/rkllm_accuracy_analysis.c
 */
int rkllm_accuracy_analysis(void* param_1, void* param_2, void* param_3, void* param_4) {
    rkllm::rkllm_log("W", "rkllm_accuracy_analysis: not implemented");
    return 0;
}

} // extern "C"
