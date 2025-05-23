# decompile-rkllm
Make closed-source code open to the web for RKLLM inference firmware


# Current status

1M lines of C code de-compiled. Most are gibbrish. However a few insights:
- This is a fork of llama.cpp (proven by the class names, model config, etc.)
- Most functions are communicated through memory addresses (not sure if it's just a product of de-compilation)
- Many limitations are put in place just to-be-safe

# Next
I started the project with the hope to add more model support to rkllm, like how the llama.cpp team did.
Now, I'm pivoting away slightly :)
