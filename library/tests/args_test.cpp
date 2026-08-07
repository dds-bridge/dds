/// @file args_test.cpp
/// @brief Unit tests for dtest option parsing and `-f` path resolution.

#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#include <errno.h>
#include <sys/stat.h>
#else
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "args.hpp"
#include "cst.hpp"

OptionsType options;

namespace
{

#ifdef _WIN32
int change_dir(const char* path)
{
  return _chdir(path);
}

std::string current_dir()
{
  std::vector<char> buf(4096);
  if (_getcwd(buf.data(), static_cast<int>(buf.size())) == nullptr)
    return {};
  return buf.data();
}
#else
int change_dir(const char* path)
{
  return chdir(path);
}

std::string current_dir()
{
  std::vector<char> buf(4096);
  if (getcwd(buf.data(), buf.size()) == nullptr)
    return {};
  return buf.data();
}
#endif

bool path_is_directory(const std::string& path)
{
  struct stat st;
  if (stat(path.c_str(), &st) != 0)
    return false;
#ifdef _WIN32
  return (st.st_mode & S_IFMT) == S_IFDIR;
#else
  return S_ISDIR(st.st_mode);
#endif
}

/// Creates a directory. Succeeds if it already exists as a directory;
/// fails if the path exists as a non-directory or on other errors.
bool make_dir(const std::string& path)
{
#ifdef _WIN32
  if (_mkdir(path.c_str()) == 0)
    return true;
#else
  if (mkdir(path.c_str(), 0755) == 0)
    return true;
#endif
  return path_is_directory(path);
}

void set_env_var(const char* name, const char* value)
{
#ifdef _WIN32
  _putenv_s(name, value != nullptr ? value : "");
#else
  if (value == nullptr || value[0] == '\0')
    unsetenv(name);
  else
    setenv(name, value, 1);
#endif
}

/// Saves and restores one environment variable for the lifetime of the guard.
class EnvVarGuard
{
 public:
  explicit EnvVarGuard(const char* name)
    : name_(name)
  {
    const char* prev = std::getenv(name_);
    if (prev != nullptr)
    {
      had_value_ = true;
      previous_ = prev;
    }
  }

  ~EnvVarGuard()
  {
    if (had_value_)
      set_env_var(name_, previous_.c_str());
    else
      set_env_var(name_, nullptr);
  }

  EnvVarGuard(const EnvVarGuard&) = delete;
  auto operator=(const EnvVarGuard&) -> EnvVarGuard& = delete;

  void set(const char* value) const
  {
    set_env_var(name_, value);
  }

 private:
  const char* name_;
  bool had_value_ = false;
  std::string previous_;
};

std::string make_temp_input_file()
{
  const std::string path =
    std::string(::testing::TempDir()) + "args_test_input.txt";
  std::ofstream out(path);
  out << "placeholder\n";
  return path;
}

/// Compare paths ignoring '\\' vs '/' so Windows TempDir / resolver mixes match.
bool same_path(std::string a, std::string b)
{
  for (char& c : a)
  {
    if (c == '\\')
      c = '/';
  }
  for (char& c : b)
  {
    if (c == '\\')
      c = '/';
  }
  return a == b;
}

/// Temporary tree: `{root}/hands/list{N}.txt` and
/// `{root}/bazel-bin/library/tests/` (binary path only; no real binary).
class HandsLayoutFixture : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    original_cwd_ = current_dir();
    ASSERT_FALSE(original_cwd_.empty());

    root_ = std::string(::testing::TempDir()) + "dtest_hands_layout/";
    ASSERT_TRUE(make_dir(root_));
    ASSERT_TRUE(make_dir(root_ + "hands"));
    ASSERT_TRUE(make_dir(root_ + "bazel-bin"));
    ASSERT_TRUE(make_dir(root_ + "bazel-bin/library"));
    ASSERT_TRUE(make_dir(root_ + "bazel-bin/library/tests"));

    {
      std::ofstream out(root_ + "hands/list42.txt");
      out << "placeholder\n";
    }

    binary_path_ = root_ + "bazel-bin/library/tests/dtest";
  }

  void TearDown() override
  {
    if (!original_cwd_.empty())
      change_dir(original_cwd_.c_str());
  }

  std::string root_;
  std::string binary_path_;
  std::string original_cwd_;
};

}  // namespace

TEST(Args, MakeDirSucceedsWhenDirectoryAlreadyExists)
{
  const std::string dir =
    std::string(::testing::TempDir()) + "args_make_dir_ok/";
  ASSERT_TRUE(make_dir(dir));
  EXPECT_TRUE(make_dir(dir));
}

TEST(Args, MakeDirFailsWhenPathIsExistingFile)
{
  const std::string path =
    std::string(::testing::TempDir()) + "args_make_dir_file";
  {
    std::ofstream out(path);
    out << "not-a-directory\n";
  }
  EXPECT_FALSE(make_dir(path));
}

TEST(Args, UnknownMinMaxFlagsAreRejected)
{
  // Former --min/--max batch-extreme flags must not be accepted.
  // read_args prints to cout and exits(0); death tests only match stderr.
  const std::string path = make_temp_input_file();
  char arg0[] = "dtest";
  char arg_f[] = "-f";
  char arg_min[] = "--min";
  char arg_max[] = "--max";
  char* argv_min[] = {arg0, arg_f, const_cast<char*>(path.c_str()), arg_min};
  char* argv_max[] = {arg0, arg_f, const_cast<char*>(path.c_str()), arg_max};
  EXPECT_EXIT(read_args(4, argv_min), ::testing::ExitedWithCode(0), ".*");
  EXPECT_EXIT(read_args(4, argv_max), ::testing::ExitedWithCode(0), ".*");
}

TEST(Args, ResolvePrefersLiteralExistingPath)
{
  const std::string path = make_temp_input_file();
  EXPECT_EQ(resolve_dtest_input_file(path, "dtest"), path);
}

TEST_F(HandsLayoutFixture, ResolveNumericUsesHandsUnderCwd)
{
  ASSERT_EQ(change_dir(root_.c_str()), 0);
  EXPECT_EQ(resolve_dtest_input_file("42", "dtest"), "hands/list42.txt");
}

TEST_F(HandsLayoutFixture, ResolveNumericFallsBackRelativeToBinary)
{
  // CWD has no hands/; argv0 points at the usual bazel-bin layout.
  ASSERT_EQ(change_dir(original_cwd_.c_str()), 0);
  EXPECT_TRUE(same_path(
    resolve_dtest_input_file("42", binary_path_),
    root_ + "hands/list42.txt"));
}

TEST_F(HandsLayoutFixture, ResolveNumericUsesBazelWorkingDirectory)
{
  // bazel run sets CWD to the runfiles tree (no hands/) and exports
  // BUILD_WORKING_DIRECTORY as the invoke-time shell cwd.
  const std::string runfiles =
    std::string(::testing::TempDir()) + "dtest_hands_runfiles/";
  ASSERT_TRUE(make_dir(runfiles));
  ASSERT_EQ(change_dir(runfiles.c_str()), 0);

  const EnvVarGuard working("BUILD_WORKING_DIRECTORY");
  const EnvVarGuard workspace("BUILD_WORKSPACE_DIRECTORY");
  working.set(root_.c_str());
  workspace.set(nullptr);

  EXPECT_TRUE(same_path(
    resolve_dtest_input_file("42", "dtest"),
    root_ + "hands/list42.txt"));
}

TEST_F(HandsLayoutFixture, ResolveNumericUsesBazelWorkspaceDirectory)
{
  const std::string runfiles =
    std::string(::testing::TempDir()) + "dtest_hands_runfiles_ws/";
  ASSERT_TRUE(make_dir(runfiles));
  ASSERT_EQ(change_dir(runfiles.c_str()), 0);

  const EnvVarGuard working("BUILD_WORKING_DIRECTORY");
  const EnvVarGuard workspace("BUILD_WORKSPACE_DIRECTORY");
  working.set(nullptr);
  workspace.set(root_.c_str());

  EXPECT_TRUE(same_path(
    resolve_dtest_input_file("42", "dtest"),
    root_ + "hands/list42.txt"));
}

TEST_F(HandsLayoutFixture, ResolveNumericWithRelativeArgv0FromOtherCwd)
{
  // Mimic running `../bazel-bin/library/tests/dtest -f 42` from a sibling of
  // the repo root: argv0 is relative, CWD is not the repo root.
  const std::string sibling =
    std::string(::testing::TempDir()) + "dtest_hands_sibling/";
  ASSERT_TRUE(make_dir(sibling));
  ASSERT_EQ(change_dir(sibling.c_str()), 0);

  // root_ and sibling share the same parent (TempDir), so this relative
  // argv0 reaches the fixture binary path.
  const std::string rel_argv0 =
    "../dtest_hands_layout/bazel-bin/library/tests/dtest";
  EXPECT_TRUE(same_path(
    resolve_dtest_input_file("42", rel_argv0),
    root_ + "hands/list42.txt"));
}

TEST_F(HandsLayoutFixture, ResolveNumericPrefersCwdOverBinaryRelative)
{
  // A different list under cwd must win even when the binary-relative file
  // also exists.
  const std::string other_root =
    std::string(::testing::TempDir()) + "dtest_hands_cwd_wins/";
  ASSERT_TRUE(make_dir(other_root));
  ASSERT_TRUE(make_dir(other_root + "hands"));
  const std::string cwd_file = other_root + "hands/list42.txt";
  {
    std::ofstream out(cwd_file);
    out << "from-cwd\n";
  }

  ASSERT_EQ(change_dir(other_root.c_str()), 0);
  EXPECT_EQ(resolve_dtest_input_file("42", binary_path_), "hands/list42.txt");
}

TEST(Args, ResolveReturnsEmptyWhenMissing)
{
  EXPECT_TRUE(resolve_dtest_input_file("no-such-list-999001", "dtest").empty());
}

TEST_F(HandsLayoutFixture, ResolveRejectsDirectoryAsLiteralPath)
{
  // -f is an input *file*; an existing directory must not short-circuit
  // resolution (otherwise read_file later fails with a confusing parse error).
  ASSERT_EQ(change_dir(root_.c_str()), 0);
  EXPECT_TRUE(resolve_dtest_input_file("hands", "dtest").empty());
  EXPECT_TRUE(resolve_dtest_input_file(root_ + "hands", "dtest").empty());
}

TEST(Args, AbsolutePathDetection)
{
  EXPECT_FALSE(is_dtest_absolute_path("tmp/dtest"));
  EXPECT_FALSE(is_dtest_absolute_path(""));
#ifndef _WIN32
  EXPECT_TRUE(is_dtest_absolute_path("/tmp/dtest"));
  // On POSIX, leading backslash is not absolute.
  EXPECT_FALSE(is_dtest_absolute_path("\\tmp\\dtest"));
#else
  // Drive-relative (no root separator after the colon) is not absolute.
  EXPECT_FALSE(is_dtest_absolute_path("C:bin\\dtest"));
  EXPECT_FALSE(is_dtest_absolute_path("C:bin/dtest"));
  EXPECT_FALSE(is_dtest_absolute_path("C:"));
  EXPECT_TRUE(is_dtest_absolute_path("C:\\bin\\dtest"));
  EXPECT_TRUE(is_dtest_absolute_path("C:/bin/dtest"));
  EXPECT_TRUE(is_dtest_absolute_path("c:\\"));
  // Current-drive rooted (leading single separator) is absolute on Windows.
  EXPECT_TRUE(is_dtest_absolute_path("\\repo\\bazel-bin\\dtest"));
  EXPECT_TRUE(is_dtest_absolute_path("/repo/bazel-bin/dtest"));
  EXPECT_TRUE(is_dtest_absolute_path("\\"));
  EXPECT_TRUE(is_dtest_absolute_path("/"));
  EXPECT_TRUE(is_dtest_absolute_path("\\\\server\\share"));
  EXPECT_TRUE(is_dtest_absolute_path("\\\\server\\share\\bazel-bin\\dtest"));
  EXPECT_TRUE(is_dtest_absolute_path("//server/share/dtest"));
  EXPECT_FALSE(is_dtest_absolute_path("\\\\server"));
  EXPECT_FALSE(is_dtest_absolute_path("\\\\server\\"));
#endif
}

#ifdef _WIN32
TEST_F(HandsLayoutFixture, ResolveNumericWithDriveRelativeArgv0)
{
  // Sit in the fake binary dir so cwd has no hands/ (otherwise the numeric
  // lookup would short-circuit before argv0). "X:dtest" is drive-relative to
  // that directory — not rooted at X:\.
  const std::string bin_dir = root_ + "bazel-bin/library/tests";
  ASSERT_EQ(change_dir(bin_dir.c_str()), 0);
  ASSERT_GE(root_.size(), 2u);
  ASSERT_EQ(root_[1], ':');

  const EnvVarGuard working("BUILD_WORKING_DIRECTORY");
  const EnvVarGuard workspace("BUILD_WORKSPACE_DIRECTORY");
  working.set(nullptr);
  workspace.set(nullptr);

  const std::string drive_rel = std::string(1, root_[0]) + ":dtest";
  EXPECT_TRUE(same_path(
    resolve_dtest_input_file("42", drive_rel),
    root_ + "hands/list42.txt"));
}

TEST_F(HandsLayoutFixture, ResolveNumericWithCurrentDriveRootedArgv0)
{
  // "\path\..." is absolute on the current drive; do not prepend cwd.
  ASSERT_GE(root_.size(), 2u);
  ASSERT_EQ(root_[1], ':');
  const std::string sibling =
    std::string(::testing::TempDir()) + "dtest_hands_rootrel_cwd/";
  ASSERT_TRUE(make_dir(sibling));
  ASSERT_EQ(change_dir(sibling.c_str()), 0);

  const EnvVarGuard working("BUILD_WORKING_DIRECTORY");
  const EnvVarGuard workspace("BUILD_WORKSPACE_DIRECTORY");
  working.set(nullptr);
  workspace.set(nullptr);

  // Drop the drive letter so argv0 is current-drive rooted.
  const std::string root_rel_argv0 =
    root_.substr(2) + "bazel-bin/library/tests/dtest";
  ASSERT_TRUE(
    root_rel_argv0[0] == '\\' || root_rel_argv0[0] == '/');
  EXPECT_TRUE(same_path(
    resolve_dtest_input_file("42", root_rel_argv0),
    root_ + "hands/list42.txt"));
}
#endif
