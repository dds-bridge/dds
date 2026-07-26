"""Shared attributes for WebAssembly (Emscripten) builds."""

# Emscripten link flags for cc_binary targets wrapped by wasm_cc_binary.
# Pair with wasm_cc_binary(threads = "emscripten") so the toolchain also
# passes -pthread / USE_PTHREADS (SharedArrayBuffer + atomics).
WASM_LINKOPTS = [
    "-sWASM=1",
    "-fwasm-exceptions",
    # Growth + pthreads is intentional for DDS TT heaps; silence the advisory
    # about slower non-wasm (JS) paths when the SharedArrayBuffer grows.
    "-Wno-pthreads-mem-growth",
    "-sALLOW_MEMORY_GROWTH=1",
    "-sINITIAL_MEMORY=268435456",
    # DDS search recursion needs more than Emscripten's 64KB default stack.
    "-sSTACK_SIZE=8388608",
    # Pre-spawn a modest worker pool for browser responsiveness. Counts above
    # this still allocate on demand; Node CLIs must shut the pool down cleanly
    # before exit so pending Worker "loaded" messages are not raced with
    # terminateAllThreads.
    "-sPTHREAD_POOL_SIZE=8",
]
