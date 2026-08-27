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
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t * data, size_t size) -> int;

// Every harness in this package defines this, even when it has nothing to set
// up. libFuzzer treats it as optional via a weak symbol, but weak references
// are not portable across ELF and Mach-O linkers, so it is required here.
extern "C" auto LLVMFuzzerInitialize(int * argc, char *** argv) -> int;

namespace {

namespace fs = std::filesystem;

/* Corpus directories arrive as workspace-relative paths and must be resolved
   through the runfiles. Mirrors library/tests/test_dtest_nothing_makes.py:
   prefer a runfiles tree (RUNFILES_DIR / TEST_SRCDIR), and fall back to
   RUNFILES_MANIFEST_FILE, which is what Windows uses when symlinked runfiles
   are disabled and no tree exists on disk. */

auto env_path(char const * name) -> std::string
{
  char const * value = std::getenv(name);
  return value == nullptr ? std::string() : std::string(value);
}

/// Files under a runfiles tree, if one contains `relpath` as a directory.
auto files_from_tree(std::string const & relpath) -> std::vector<fs::path>
{
  std::vector<fs::path> found;

  for (char const * key : {"RUNFILES_DIR", "TEST_SRCDIR"})
  {
    std::string const root = env_path(key);
    if (root.empty())
      continue;

    for (fs::path const & candidate :
         {fs::path(root) / relpath, fs::path(root) / "_main" / relpath})
    {
      std::error_code ec;
      if (!fs::is_directory(candidate, ec))
        continue;

      for (auto const & entry : fs::recursive_directory_iterator(candidate, ec))
        if (entry.is_regular_file())
          found.push_back(entry.path());

      if (!found.empty())
        return found;
    }
  }

  return found;
}

/// Files under `relpath` named by the runfiles manifest (Windows).
auto files_from_manifest(std::string const & relpath) -> std::vector<fs::path>
{
  std::vector<fs::path> found;

  std::string const manifest = env_path("RUNFILES_MANIFEST_FILE");
  if (manifest.empty())
    return found;

  std::ifstream in(manifest);
  if (!in)
    return found;

  // Manifest keys use forward slashes and may or may not carry the repo name.
  std::string const with_repo = "_main/" + relpath + "/";
  std::string const bare = relpath + "/";

  std::string line;
  while (std::getline(in, line))
  {
    if (line.empty() || line.front() == '[' || line.front() == ' ')
      continue;

    auto const space = line.find(' ');
    if (space == std::string::npos)
      continue;

    std::string const key = line.substr(0, space);
    std::string const value = line.substr(space + 1);
    if (value.empty())
      continue;

    if (key.rfind(with_repo, 0) != 0 && key.rfind(bare, 0) != 0)
      continue;

    std::error_code ec;
    if (fs::is_regular_file(value, ec))
      found.emplace_back(value);
  }

  return found;
}

/// Every file under `arg`, whether it names a runfiles directory, a plain
/// directory, or a single file.
auto corpus_files(std::string const & arg) -> std::vector<fs::path>
{
  std::vector<fs::path> found = files_from_tree(arg);
  if (!found.empty())
    return found;

  found = files_from_manifest(arg);
  if (!found.empty())
    return found;

  // Direct invocation from a shell, where the path is simply on disk.
  std::error_code ec;
  if (fs::is_directory(arg, ec))
  {
    for (auto const & entry : fs::recursive_directory_iterator(arg, ec))
      if (entry.is_regular_file())
        found.push_back(entry.path());
  }
  else if (fs::is_regular_file(arg, ec))
  {
    found.emplace_back(arg);
  }

  return found;
}

auto run_one(fs::path const & path) -> bool
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
  // or harnesses relying on it (e.g. InitializeStaticMemory) run unconfigured.
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
    std::vector<fs::path> const found = corpus_files(argv[i]);

    if (found.empty())
    {
      std::fprintf(stderr, "no corpus files under: %s\n", argv[i]);
      ok = false;
      continue;
    }

    for (fs::path const & path : found)
    {
      ok = run_one(path) && ok;
      files++;
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
