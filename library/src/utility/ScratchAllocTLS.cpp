#include "utility/ScratchAllocTLS.hpp"

namespace dds {
namespace tls {

static thread_local AllocFn g_alloc = nullptr;
static thread_local void* g_ctx = nullptr;

void SetAlloc(AllocFn fn, void* ctx) noexcept {
  g_alloc = fn;
  g_ctx = ctx;
}

void ResetAlloc() noexcept {
  g_alloc = nullptr;
  g_ctx = nullptr;
}

void* TryAlloc(std::size_t size, std::size_t align) noexcept {
  if (!g_alloc) return nullptr;
  return g_alloc(size, align, g_ctx);
}

} // namespace tls
} // namespace dds
