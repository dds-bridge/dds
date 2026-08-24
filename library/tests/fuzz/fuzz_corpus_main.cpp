/*
   DDS, a bridge double dummy solver.

   See LICENSE and README.
*/

/// @file fuzz_corpus_main.cpp
/// @brief Standalone driver that replays a seed corpus through a fuzz harness.
///
/// libFuzzer is only available under --config=fuzz. This driver lets the same
/// LLVMFuzzerTestOneInput() harnesses run as ordinary cc_tests on every
/// platform and under --config=asan/ubsan, so the corpus acts as a regression
/// suite even where no fuzzer is linked.
///
/// Each argument is a file or a directory; directories are walked recursively.

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t * data, size_t size) -> int;

// Every harness in this package defines this, even when it has nothing to set
// up. libFuzzer treats it as optional via a weak symbol, but weak references
// are not portable across ELF and Mach-O linkers, so it is required here.
extern "C" auto LLVMFuzzerInitialize(int * argc, char *** argv) -> int;

namespace {

auto run_one(std::filesystem::path const & path) -> bool
{
  std::ifstream in(path, std::ios::binary);
  if (!in)
  {
    std::fprintf(stderr, "cannot open %s\n", path.string().c_str());
    return false;
  }

  std::vector<uint8_t> const bytes(
    (std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

  LLVMFuzzerTestOneInput(bytes.data(), bytes.size());
  return true;
}

}  // namespace

auto main(int argc, char ** argv) -> int
{
  // libFuzzer calls this before the first input; the replay driver must too,
  // or harnesses relying on it (e.g. SetMaxThreads) run unconfigured.
  LLVMFuzzerInitialize(&argc, &argv);

  // Degenerate inputs every harness must survive, independent of the corpus.
  uint8_t const zero[32] = {0};
  uint8_t const ones[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  LLVMFuzzerTestOneInput(nullptr, 0);
  LLVMFuzzerTestOneInput(zero, sizeof(zero));
  LLVMFuzzerTestOneInput(ones, sizeof(ones));

  int files = 0;
  bool ok = true;

  for (int i = 1; i < argc; i++)
  {
    std::filesystem::path const root(argv[i]);
    std::error_code ec;

    if (std::filesystem::is_directory(root, ec))
    {
      for (auto const & entry :
           std::filesystem::recursive_directory_iterator(root, ec))
      {
        if (!entry.is_regular_file())
          continue;
        ok = run_one(entry.path()) && ok;
        files++;
      }
    }
    else if (std::filesystem::is_regular_file(root, ec))
    {
      ok = run_one(root) && ok;
      files++;
    }
    else
    {
      std::fprintf(stderr, "no such corpus path: %s\n", argv[i]);
      ok = false;
    }
  }

  // A corpus that silently resolves to nothing would make this test vacuous.
  if (argc > 1 && files == 0)
  {
    std::fprintf(stderr, "corpus resolved to 0 files\n");
    return 1;
  }

  std::printf("replayed %d corpus file(s)\n", files);
  return ok ? 0 : 1;
}
