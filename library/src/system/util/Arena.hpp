#ifndef DDS_SYSTEM_UTIL_ARENA_H
#define DDS_SYSTEM_UTIL_ARENA_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace dds {

// A simple bump-pointer arena with optional fallback allocations.
// - allocate(size, align): returns pointer into internal buffer when possible;
//   falls back to heap for oversized/overflow allocations and tracks them.
// - reset(): frees all bump allocations by rewinding, and deletes fallbacks.
// - thread-unsafe by design; intended for per-instance, per-solve reuse.
class Arena {
public:
  explicit Arena(std::size_t capacity_bytes)
  : buf_(capacity_bytes), off_(0) {}

  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;
  Arena(Arena&&) = default;
  Arena& operator=(Arena&&) = default;

  struct AllocOpts { std::size_t size; std::size_t align = alignof(std::max_align_t); };

  void* allocate(AllocOpts opts) {
    std::size_t size = opts.size;
    std::size_t align = opts.align;
    if (align == 0) align = alignof(std::max_align_t);
    std::size_t base = reinterpret_cast<std::size_t>(buf_.data());
    std::size_t cur = base + off_;
    std::size_t aligned = (cur + (align - 1)) & ~(align - 1);
    std::size_t new_off = (aligned - base) + size;
    if (new_off <= buf_.size()) {
      off_ = new_off;
      return reinterpret_cast<void*>(aligned);
    }
    // Fallback: allocate separately and free on reset.
    auto* mem = new std::uint8_t[size];
    fallbacks_.push_back(mem);
    return mem;
  }

  template <class T>
  T* allocate_array(std::size_t count) {
    return static_cast<T*>(allocate({sizeof(T) * count, alignof(T)}));
  }

  void reset() {
    off_ = 0;
    for (auto* p : fallbacks_) delete[] p;
    fallbacks_.clear();
  }

  std::size_t capacity() const { return buf_.size(); }
  std::size_t offset() const { return off_; }
  std::size_t fallback_count() const { return fallbacks_.size(); }

private:
  std::vector<std::uint8_t> buf_;
  std::size_t off_ = 0;
  std::vector<std::uint8_t*> fallbacks_;
};

} // namespace dds

#endif // DDS_SYSTEM_UTIL_ARENA_H
