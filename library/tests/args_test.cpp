/// @file args_test.cpp
/// @brief Unit tests for dtest option parsing and `-f` path resolution.

#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#include <stdlib.h>
#else
#include <stdlib.h>
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

void make_dir(const std::string& path)
{
#ifdef _WIN32
  _mkdir(path.c_str());
#else
  mkdir(path.c_str(), 0755);
#endif
}

std::string make_temp_input_file()
{
  const std::string path =
    std::string(::testing::TempDir()) + "args_test_input.txt";
  std::ofstream out(path);
  out << "placeholder\n";
  return path;
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
    make_dir(root_);
    make_dir(root_ + "hands");
    make_dir(root_ + "bazel-bin");
    make_dir(root_ + "bazel-bin/library");
    make_dir(root_ + "bazel-bin/library/tests");

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

TEST(Args, MaxAndMinFlagsDefaultOff)
{
  const std::string path = make_temp_input_file();
  char arg0[] = "dtest";
  char arg_f[] = "-f";
  char* argv[] = {arg0, arg_f, const_cast<char*>(path.c_str())};
  read_args(3, argv);
  EXPECT_FALSE(options.show_min_);
  EXPECT_FALSE(options.show_max_);
}

TEST(Args, MaxFlagEnablesShowMax)
{
  const std::string path = make_temp_input_file();
  char arg0[] = "dtest";
  char arg_f[] = "-f";
  char arg_max[] = "--max";
  char* argv[] = {arg0, arg_f, const_cast<char*>(path.c_str()), arg_max};
  read_args(4, argv);
  EXPECT_TRUE(options.show_max_);
  EXPECT_FALSE(options.show_min_);
}

TEST(Args, MinFlagEnablesShowMin)
{
  const std::string path = make_temp_input_file();
  char arg0[] = "dtest";
  char arg_f[] = "-f";
  char arg_min[] = "--min";
  char* argv[] = {arg0, arg_f, const_cast<char*>(path.c_str()), arg_min};
  read_args(4, argv);
  EXPECT_TRUE(options.show_min_);
  EXPECT_FALSE(options.show_max_);
}

TEST(Args, MaxAndMinFlagsCanCombine)
{
  const std::string path = make_temp_input_file();
  char arg0[] = "dtest";
  char arg_f[] = "-f";
  char arg_max[] = "--max";
  char arg_min[] = "--min";
  char* argv[] = {
    arg0, arg_f, const_cast<char*>(path.c_str()), arg_max, arg_min};
  read_args(5, argv);
  EXPECT_TRUE(options.show_min_);
  EXPECT_TRUE(options.show_max_);
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
  EXPECT_EQ(
    resolve_dtest_input_file("42", binary_path_),
    root_ + "hands/list42.txt");
}

TEST_F(HandsLayoutFixture, ResolveNumericUsesBazelWorkingDirectory)
{
  // bazel run sets CWD to the runfiles tree (no hands/) and exports
  // BUILD_WORKING_DIRECTORY as the invoke-time shell cwd.
  const std::string runfiles =
    std::string(::testing::TempDir()) + "dtest_hands_runfiles/";
  make_dir(runfiles);
  ASSERT_EQ(change_dir(runfiles.c_str()), 0);
#ifdef _WIN32
  _putenv_s("BUILD_WORKING_DIRECTORY", root_.c_str());
  _putenv_s("BUILD_WORKSPACE_DIRECTORY", "");
#else
  ASSERT_EQ(setenv("BUILD_WORKING_DIRECTORY", root_.c_str(), 1), 0);
  ASSERT_EQ(unsetenv("BUILD_WORKSPACE_DIRECTORY"), 0);
#endif
  EXPECT_EQ(
    resolve_dtest_input_file("42", "dtest"),
    root_ + "hands/list42.txt");
#ifdef _WIN32
  _putenv_s("BUILD_WORKING_DIRECTORY", "");
#else
  unsetenv("BUILD_WORKING_DIRECTORY");
#endif
}

TEST_F(HandsLayoutFixture, ResolveNumericUsesBazelWorkspaceDirectory)
{
  const std::string runfiles =
    std::string(::testing::TempDir()) + "dtest_hands_runfiles_ws/";
  make_dir(runfiles);
  ASSERT_EQ(change_dir(runfiles.c_str()), 0);
#ifdef _WIN32
  _putenv_s("BUILD_WORKING_DIRECTORY", "");
  _putenv_s("BUILD_WORKSPACE_DIRECTORY", root_.c_str());
#else
  ASSERT_EQ(unsetenv("BUILD_WORKING_DIRECTORY"), 0);
  ASSERT_EQ(setenv("BUILD_WORKSPACE_DIRECTORY", root_.c_str(), 1), 0);
#endif
  EXPECT_EQ(
    resolve_dtest_input_file("42", "dtest"),
    root_ + "hands/list42.txt");
#ifdef _WIN32
  _putenv_s("BUILD_WORKSPACE_DIRECTORY", "");
#else
  unsetenv("BUILD_WORKSPACE_DIRECTORY");
#endif
}

TEST_F(HandsLayoutFixture, ResolveNumericWithRelativeArgv0FromOtherCwd)
{
  // Mimic running `../bazel-bin/library/tests/dtest -f 42` from a sibling of
  // the repo root: argv0 is relative, CWD is not the repo root.
  const std::string sibling =
    std::string(::testing::TempDir()) + "dtest_hands_sibling/";
  make_dir(sibling);
  ASSERT_EQ(change_dir(sibling.c_str()), 0);

  // root_ and sibling share the same parent (TempDir), so this relative
  // argv0 reaches the fixture binary path.
  const std::string rel_argv0 =
    "../dtest_hands_layout/bazel-bin/library/tests/dtest";
  EXPECT_EQ(
    resolve_dtest_input_file("42", rel_argv0),
    root_ + "hands/list42.txt");
}

TEST_F(HandsLayoutFixture, ResolveNumericPrefersCwdOverBinaryRelative)
{
  // A different list under cwd must win even when the binary-relative file
  // also exists.
  const std::string other_root =
    std::string(::testing::TempDir()) + "dtest_hands_cwd_wins/";
  make_dir(other_root);
  make_dir(other_root + "hands");
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
