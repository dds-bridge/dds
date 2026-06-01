"""Shared attributes for native vs WebAssembly builds."""

# Mark targets incompatible when building with --config=wasm.
NATIVE_ONLY = select({
    "//:build_wasm": ["@platforms//:incompatible"],
    "//conditions:default": [],
})

# Emscripten link flags for cc_binary targets wrapped by wasm_cc_binary.
WASM_LINKOPTS = [
    "-sWASM=1",
    "-fexceptions",
    "-sALLOW_MEMORY_GROWTH=1",
    "-sINITIAL_MEMORY=268435456",
]
