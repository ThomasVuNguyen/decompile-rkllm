/**
 * RKLLM Sampling
 * 
 * Token sampling algorithms: top-k, top-p, temperature, mirostat, etc.
 * Based on llama.cpp sampling implementation.
 */

#include "rkllm_internal.h"
#include <cmath>
#include <algorithm>
#include <random>

namespace rkllm {

// Random number generator
static std::mt19937 g_rng(std::random_device{}());

/**
 * Apply temperature scaling to logits
 */
static void apply_temperature(std::vector<Token>& candidates, float temperature) {
    if (temperature <= 0.0f) {
        return; // greedy
    }
    
    for (auto& c : candidates) {
        c.logit /= temperature;
    }
}

/**
 * Apply repetition penalty
 */
static void apply_repetition_penalty(std::vector<Token>& candidates,
                                      const std::vector<int32_t>& last_tokens,
                                      float penalty) {
    if (penalty == 1.0f || last_tokens.empty()) {
        return;
    }
    
    for (auto& c : candidates) {
        bool found = std::find(last_tokens.begin(), last_tokens.end(), c.id) != last_tokens.end();
        if (found) {
            if (c.logit > 0) {
                c.logit /= penalty;
            } else {
                c.logit *= penalty;
            }
        }
    }
}

/**
 * Softmax to convert logits to probabilities
 */
static void softmax(std::vector<Token>& candidates) {
    float max_logit = candidates[0].logit;
    for (const auto& c : candidates) {
        max_logit = std::max(max_logit, c.logit);
    }
    
    float sum = 0.0f;
    for (auto& c : candidates) {
        c.prob = expf(c.logit - max_logit);
        sum += c.prob;
    }
    
    for (auto& c : candidates) {
        c.prob /= sum;
    }
}

/**
 * Top-K sampling: keep only top K candidates
 */
static void sample_top_k(std::vector<Token>& candidates, int k) {
    if (k <= 0 || k >= (int)candidates.size()) {
        return;
    }
    
    std::partial_sort(candidates.begin(), candidates.begin() + k, candidates.end(),
        [](const Token& a, const Token& b) { return a.logit > b.logit; });
    
    candidates.resize(k);
}

/**
 * Top-P (nucleus) sampling: keep smallest set of tokens with cumulative prob >= p
 */
static void sample_top_p(std::vector<Token>& candidates, float p) {
    if (p >= 1.0f) {
        return;
    }
    
    // Sort by probability
    std::sort(candidates.begin(), candidates.end(),
        [](const Token& a, const Token& b) { return a.prob > b.prob; });
    
    float cumsum = 0.0f;
    size_t cutoff = candidates.size();
    
    for (size_t i = 0; i < candidates.size(); ++i) {
        cumsum += candidates[i].prob;
        if (cumsum >= p) {
            cutoff = i + 1;
            break;
        }
    }
    
    candidates.resize(cutoff);
}

/**
 * Mirostat sampling algorithm
 * Maintains target cross-entropy tau
 */
static int32_t sample_mirostat(std::vector<Token>& candidates, float tau, float eta, float& mu) {
    // Sort by logit descending
    std::sort(candidates.begin(), candidates.end(),
        [](const Token& a, const Token& b) { return a.logit > b.logit; });
    
    softmax(candidates);
    
    // Estimate s (power law exponent)
    float s_hat = 0.0f;
    float sum = 0.0f;
    for (size_t i = 0; i < candidates.size() && i < 100; ++i) {
        if (candidates[i].prob > 0) {
            s_hat += candidates[i].prob * logf(candidates[i].prob);
            sum += candidates[i].prob;
        }
    }
    s_hat = -s_hat / sum;
    
    // s should be around 1.0 for natural language
    float s = std::max(s_hat, 1.0f);
    
    // Compute k based on mu and s
    int k = (int)std::ceil(powf(mu, 1.0f / s) * 0.5f);
    k = std::max(1, std::min(k, (int)candidates.size()));
    
    // Sample from top k
    candidates.resize(k);
    float total = 0.0f;
    for (const auto& c : candidates) {
        total += c.prob;
    }
    
    std::uniform_real_distribution<float> dist(0.0f, total);
    float r = dist(g_rng);
    
    float cumsum = 0.0f;
    int32_t selected = candidates[0].id;
    float selected_prob = candidates[0].prob;
    
    for (const auto& c : candidates) {
        cumsum += c.prob;
        if (r <= cumsum) {
            selected = c.id;
            selected_prob = c.prob;
            break;
        }
    }
    
    // Update mu
    float surprise = -log2f(selected_prob);
    mu = mu - eta * (surprise - tau);
    
    return selected;
}

/**
 * Main sampling function
 */
int32_t rkllm_sample(RKLLMContextInternal* ctx) {
    if (!ctx || ctx->sampler.candidates.empty()) {
        return -1;
    }
    
    std::vector<Token> candidates = ctx->sampler.candidates;
    
    // Apply repetition penalty
    apply_repetition_penalty(candidates, ctx->sampler.last_tokens, 
                             ctx->params.repeat_penalty);
    
    // Apply temperature
    apply_temperature(candidates, ctx->params.temperature);
    
    int32_t selected;
    
    if (ctx->params.mirostat > 0) {
        // Mirostat sampling
        selected = sample_mirostat(candidates, ctx->params.mirostat_tau,
                                   ctx->params.mirostat_eta, ctx->sampler.mirostat_mu);
    } else {
        // Standard sampling: top-k, then top-p
        sample_top_k(candidates, ctx->params.top_k);
        softmax(candidates);
        sample_top_p(candidates, ctx->params.top_p);
        
        // Sample from remaining candidates
        if (ctx->params.temperature <= 0.0f) {
            // Greedy
            selected = candidates[0].id;
        } else {
            float total = 0.0f;
            for (const auto& c : candidates) {
                total += c.prob;
            }
            
            std::uniform_real_distribution<float> dist(0.0f, total);
            float r = dist(g_rng);
            
            float cumsum = 0.0f;
            selected = candidates[0].id;
            
            for (const auto& c : candidates) {
                cumsum += c.prob;
                if (r <= cumsum) {
                    selected = c.id;
                    break;
                }
            }
        }
    }
    
    // Add to last tokens for repetition penalty
    ctx->sampler.last_tokens.push_back(selected);
    if (ctx->sampler.last_tokens.size() > (size_t)ctx->sampler.repeat_last_n) {
        ctx->sampler.last_tokens.erase(ctx->sampler.last_tokens.begin());
    }
    
    return selected;
}

} // namespace rkllm
