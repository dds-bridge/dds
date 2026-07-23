"""Shared attributes for WebAssembly (Emscripten) builds."""

# Emscripten link flags for cc_binary targets wrapped by wasm_cc_binary.
WASM_LINKOPTS = [
    "-sWASM=1",
    "-fwasm-exceptions",
    "-sALLOW_MEMORY_GROWTH=1",
    "-sINITIAL_MEMORY=268435456",
    # DDS search recursion needs more than Emscripten's 64KB default stack.
    "-sSTACK_SIZE=8388608",
]
