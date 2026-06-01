"""Shared attributes for WebAssembly (Emscripten) builds."""

# Emscripten link flags for cc_binary targets wrapped by wasm_cc_binary.
WASM_LINKOPTS = [
    "-sWASM=1",
    "-fexceptions",
    "-sALLOW_MEMORY_GROWTH=1",
    "-sINITIAL_MEMORY=268435456",
]
