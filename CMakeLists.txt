cmake_minimum_required(VERSION 3.14)
project(open-rkllm VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Options
option(RKLLM_BUILD_SHARED "Build shared library" ON)
option(RKLLM_USE_RKNN "Link with real RKNN library" OFF)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/ghidra1        # Original rkllm.h
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/rknn_include
)

# Source files
set(RKLLM_SOURCES
    src/rkllm.cpp
    src/rkllm_context.cpp
    src/rkllm_sampling.cpp
    src/rkllm_utils.cpp
    src/rknn_backend.cpp
    src/gguf_parser.cpp
    src/transformer.cpp
    src/tokenizer.cpp
)

# Create library
if(RKLLM_BUILD_SHARED)
    add_library(rkllmrt SHARED ${RKLLM_SOURCES})
else()
    add_library(rkllmrt STATIC ${RKLLM_SOURCES})
endif()

# Link RKNN if available
if(RKLLM_USE_RKNN)
    find_library(RKNN_LIB rknnrt PATHS /usr/lib /usr/local/lib)
    if(RKNN_LIB)
        target_link_libraries(rkllmrt ${RKNN_LIB})
        target_compile_definitions(rkllmrt PRIVATE RKLLM_HAS_RKNN=1)
    endif()
endif()

target_link_libraries(rkllmrt pthread dl)

# Install
install(TARGETS rkllmrt
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(FILES ghidra1/rkllm.h DESTINATION include)
