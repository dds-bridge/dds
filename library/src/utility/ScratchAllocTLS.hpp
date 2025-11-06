#pragma once
#include <cstddef>

namespace dds {
namespace tls {

// Function pointer type for scratch allocations: (size, align, ctx) -> ptr
using AllocFn = void* (*)(std::size_t, std::size_t, void*);

// Install a thread-local allocator for scratch buffers. Pass nullptr to disable.
void SetAlloc(AllocFn fn, void* ctx) noexcept;

// Reset the thread-local allocator to disabled state.
void ResetAlloc() noexcept;

// Try to allocate a scratch buffer using the TLS allocator if installed.
// Returns nullptr if no allocator is set or allocation fails.
void* TryAlloc(std::size_t size, std::size_t align) noexcept;

} // namespace tls
} // namespace dds
