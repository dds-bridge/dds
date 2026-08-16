/* 
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund / 
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// These functions parse the command line for options.


#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <system_error>

#include "args.hpp"
#include "cst.hpp"

using std::cout;
using std::endl;
using std::setw;
using std::right;
using std::left;
using std::vector;
using std::string;
namespace fs = std::filesystem;


extern OptionsType options;

struct optEntry
{
  string shortName;
  string longName;
  unsigned numArgs;
};

#define DTEST_NUM_OPTIONS 5

enum DtestOpt
{
  OPT_FILE = 0,
  OPT_SOLVER = 1,
  OPT_NUMTHR = 2,
  OPT_MEMORY = 3,
  OPT_REPORT = 4
};

const optEntry optList[DTEST_NUM_OPTIONS] =
{
  {"f", "file", 1},
  {"s", "solver", 1},
  {"n", "numthr", 1},
  {"m", "memory", 1},
  {"r", "report", 0}
};

const vector<string> solverList =
{
  "solve",
  "calc",
  "play",
  "par",
  "dealerpar"
};

string shortOptsAll, shortOptsWithArg;

int GetNextArgToken(
  int argc,
  char * argv[]);

void SetDefaults();

bool ParseRound();


void usage(
  const char base[])
{
  const string basename = fs::path(base).filename().string();

  cout <<
    "Usage: " << basename << " [options]\n\n" <<
    "-f, --file s       Input file, or the number n;\n" <<
    "                   Relative paths (and '100' → hands/list100.txt) are\n" <<
    "                   resolved under the current directory, then under\n" <<
    "                   BUILD_WORKING_DIRECTORY / BUILD_WORKSPACE_DIRECTORY\n" <<
    "                   (bazel run), else relative to the dtest binary\n" <<
    "                   (bazel-bin/library/tests/).\n" <<
    "                   (Default: input.txt)\n" <<
    "\n" <<
    "-s, --solver       One of: solve, calc, play, par, dealerpar.\n" <<
    "                   (Default: solve)\n" <<
    "\n" <<
    "-n, --numthr n     Worker threads for solve/calc/play batches.\n" <<
    "                   0 = auto (hardware concurrency), 1 = sequential.\n" <<
    "                   (Default: 0)\n" <<
    "\n" <<
    "-m, --memory n     Total DDS memory size in MB (legacy option).\n" <<
    "                   (Default: 0 uses DDS/library defaults; when using\n" <<
    "                   the modern SolverContext API, prefer configuring\n" <<
    "                   memory via SolverConfig instead of this option.)\n" <<
    "\n" <<
    "-r, --report       Print per-deal timings in ms (two decimals) for every\n" <<
    "                   hand in the input (solve mode), longest first, plus\n" <<
    "                   a min/max/mean/median/stddev summary.\n" <<
    "\n" <<
    endl;
}


int nextToken = 1;
char * optarg;

int GetNextArgToken(
  int argc,
  char * argv[])
{
  // 0 means done, -1 means error.

  if (nextToken >= argc)
    return 0;

  string str(argv[nextToken]);
  if (str[0] != '-' || str.size() == 1)
    return -1;

  if (str[1] == '-')
  {
    if (str.size() == 2)
      return -1;
    str.erase(0, 2);
  }
  else if (str.size() == 2)
    str.erase(0, 1);
  else
    return -1;

  for (unsigned i = 0; i < DTEST_NUM_OPTIONS; i++)
  {
    const bool short_ok =
      !optList[i].shortName.empty() && str == optList[i].shortName;
    if (short_ok || str == optList[i].longName)
    {
      if (optList[i].numArgs == 1)
      {
        if (nextToken+1 >= argc)
          return -1;

        optarg = argv[nextToken+1];
        nextToken += 2;
      }
      else
        nextToken++;

      // Return 1-based option index so 0 can mean "done".
      return static_cast<int>(i) + 1;
    }
  }

  return -1;
}


void SetDefaults()
{
  options.fname_ = "input.txt";
  options.solver_ = Solver::DTEST_SOLVER_SOLVE;
  options.num_threads_ = 0;
  options.memory_mb_ = 0;
  options.report_slow_boards_ = false;
}


namespace
{

#ifdef _WIN32
bool is_unc_path(const string& path)
{
  if (path.size() < 5)
    return false;
  const bool unc_slash =
    (path[0] == '\\' && path[1] == '\\') ||
    (path[0] == '/' && path[1] == '/');
  if (!unc_slash)
    return false;

  size_t i = 2;
  while (i < path.size() && (path[i] == '\\' || path[i] == '/'))
    ++i;
  if (i >= path.size())
    return false;

  const size_t server_start = i;
  while (i < path.size() && path[i] != '\\' && path[i] != '/')
    ++i;
  if (i == server_start || i >= path.size())
    return false;

  while (i < path.size() && (path[i] == '\\' || path[i] == '/'))
    ++i;
  if (i >= path.size())
    return false;

  const size_t share_start = i;
  while (i < path.size() && path[i] != '\\' && path[i] != '/')
    ++i;
  return i > share_start;
}
#endif


bool is_absolute_path(const string& path)
{
  if (path.empty())
    return false;
#ifdef _WIN32
  if (is_unc_path(path))
    return true;
  // Current-drive rooted: "\foo" or "/foo" (single leading separator, not UNC).
  if ((path[0] == '\\' || path[0] == '/') &&
    (path.size() == 1 || (path[1] != '\\' && path[1] != '/')))
  {
    return true;
  }
  // Drive-rooted absolute: "C:\..." or "C:/...". "C:foo" is drive-relative.
  return path.size() >= 3 &&
    std::isalpha(static_cast<unsigned char>(path[0])) &&
    path[1] == ':' &&
    (path[2] == '\\' || path[2] == '/');
#else
  return path[0] == '/';
#endif
}


// Collapse "." / ".." path segments without resolving symlinks.
string normalize_logical_path(const string& path)
{
  if (path.empty())
    return path;
  return fs::path(path).lexically_normal().make_preferred().string();
}


bool path_exists(const fs::path& path)
{
  std::error_code ec;
  // -f / resolve_dtest_input_file expect a readable input *file*; directories
  // must not count (exists() is true for them and leads to a later parse error).
  return fs::is_regular_file(path, ec);
}


// Logical absolute path for argv0 without resolving symlinks (so climbing out
// of bazel-bin still lands on the workspace, not the execroot).
string absolute_path_logical(const string& path)
{
  std::error_code ec;
  const fs::path cwd = fs::current_path(ec);

  if (is_absolute_path(path))
  {
#ifdef _WIN32
    // Current-drive rooted "\foo" → "X:\foo" using the cwd drive letter.
    // std::filesystem treats these as relative (no root-name), so handle here.
    if ((path[0] == '\\' || path[0] == '/') &&
      (path.size() == 1 || (path[1] != '\\' && path[1] != '/')))
    {
      if (!ec && cwd.has_root_name())
        return normalize_logical_path(cwd.root_name().string() + path);
    }
#endif
    return normalize_logical_path(path);
  }

  if (ec)
    return normalize_logical_path(path);

#ifdef _WIN32
  // Drive-relative "C:foo": resolve against cwd when cwd is on the same drive.
  if (path.size() >= 2 &&
    std::isalpha(static_cast<unsigned char>(path[0])) &&
    path[1] == ':')
  {
    const string cwd_s = cwd.string();
    if (cwd_s.size() >= 2 &&
      std::tolower(static_cast<unsigned char>(cwd_s[0])) ==
        std::tolower(static_cast<unsigned char>(path[0])) &&
      cwd_s[1] == ':')
    {
      return normalize_logical_path((cwd / path.substr(2)).string());
    }
    return normalize_logical_path(path);
  }
#endif

  if (path.empty())
    return normalize_logical_path(cwd.string());
  return normalize_logical_path((cwd / path).string());
}

}  // namespace


bool is_dtest_absolute_path(const string& path)
{
  return is_absolute_path(path);
}


string resolve_dtest_input_file(
  const string& arg,
  const string& argv0)
{
  if (path_exists(arg))
    return arg;

  const string list_name = "list" + arg + ".txt";
  // Keep generic separators so cwd hits match the documented hands/listN.txt form.
  const string cwd_candidate =
    (fs::path("hands") / list_name).generic_string();
  if (path_exists(cwd_candidate))
    return cwd_candidate;

  // bazel run moves CWD into the runfiles tree; it exports the invoke-time
  // shell cwd and the workspace root so relative -f paths still resolve.
  auto from_env_dir = [&](const char* env_name, const fs::path& rel) -> string
  {
    const char* dir = std::getenv(env_name);
    if (dir == nullptr || dir[0] == '\0')
      return string();
    const string candidate =
      normalize_logical_path((fs::path(dir) / rel).string());
    if (path_exists(candidate))
      return candidate;
    return string();
  };

  // Climb parents in the path *string* (do not use "/../" with filesystem
  // resolution — that follows a bazel-bin symlink into the execroot and misses
  // the workspace hands/ directory). bazel-bin/library/tests/dtest → four
  // parent_path steps to the repo root.
  auto workspace_root_from_argv0 = [&]() -> fs::path
  {
    fs::path dir(absolute_path_logical(argv0));
    for (unsigned i = 0; i < 4; ++i)
    {
      const fs::path parent = dir.parent_path();
      if (parent.empty())
        return {};
      dir = parent;
    }
    return dir;
  };

  if (!is_absolute_path(arg))
  {
    if (const string found =
          from_env_dir("BUILD_WORKING_DIRECTORY", arg); !found.empty())
    {
      return found;
    }
    if (const string found =
          from_env_dir("BUILD_WORKSPACE_DIRECTORY", arg); !found.empty())
    {
      return found;
    }
  }

  const fs::path list_rel = fs::path("hands") / list_name;
  if (const string found =
        from_env_dir("BUILD_WORKING_DIRECTORY", list_rel); !found.empty())
  {
    return found;
  }
  if (const string found =
        from_env_dir("BUILD_WORKSPACE_DIRECTORY", list_rel); !found.empty())
  {
    return found;
  }

  const fs::path root = workspace_root_from_argv0();
  if (root.empty())
    return string();

  if (!is_absolute_path(arg))
  {
    const string bin_literal =
      normalize_logical_path((root / arg).string());
    if (path_exists(bin_literal))
      return bin_literal;
  }

  const string bin_candidate =
    normalize_logical_path((root / "hands" / list_name).string());
  if (path_exists(bin_candidate))
    return bin_candidate;

  return string();
}


void print_options()
{
  cout << left;
  cout << setw(12) << "file" << 
    setw(12) <<  options.fname_ << "\n";
  cout << setw(12) << "solver" << setw(12) <<  
    solverList[static_cast<size_t>(options.solver_)] << "\n";
  cout << setw(12) << "threads" << setw(12) <<  
    options.num_threads_ << "\n";
  cout << setw(12) << "memory" << setw(12) <<  
    options.memory_mb_ << " MB\n";
  cout << "\n" << right;
}


void read_args(
  int argc,
  char * argv[])
{
  nextToken = 1;
  shortOptsAll.clear();
  shortOptsWithArg.clear();

  for (unsigned i = 0; i < DTEST_NUM_OPTIONS; i++)
  {
    shortOptsAll += optList[i].shortName;
    if (optList[i].numArgs)
      shortOptsWithArg += optList[i].shortName;
  }

  if (argc == 1)
  {
    usage(argv[0]);
    exit(0);
  }

  SetDefaults();

  int c, m = 0;
  bool errFlag = false, matchFlag;
  string stmp;
  char * ctmp;

  while ((c = GetNextArgToken(argc, argv)) > 0)
  {
    switch(c - 1)
    {
      case OPT_FILE:
      {
        const string resolved =
          resolve_dtest_input_file(string(optarg), string(argv[0]));
        if (!resolved.empty())
        {
          options.fname_ = resolved;
          break;
        }

        cout << "Input file '" << optarg << "' not found\n";
        cout << "Also tried that path under the current directory, "
          "BUILD_WORKING_DIRECTORY, BUILD_WORKSPACE_DIRECTORY, "
          "and relative to the dtest binary; "
          "for numeric -f N, also hands/listN.txt\n";
        nextToken -= 2;
        errFlag = true;
        break;
      }

      case OPT_SOLVER:
        matchFlag = false;
        stmp = optarg;
        transform(stmp.begin(), stmp.end(), stmp.begin(),
            [](unsigned char c) { return static_cast<char>(::tolower(c)); });

        for (unsigned i = 0; i < static_cast<unsigned>(Solver::DTEST_SOLVER_SIZE) && ! matchFlag; i++)
        {
          string s = solverList[i];
          transform(s.begin(), s.end(), s.begin(),
              [](unsigned char c) { return static_cast<char>(::tolower(c)); });
          if (stmp == s)
          {
            m = static_cast<int>(i);
            matchFlag = true;
          }
        }

        if (matchFlag)
          options.solver_ = static_cast<Solver>(m);
        else
        {
          cout << "Solver '" << optarg << "' not found\n";
          nextToken -= 2;
          errFlag = true;
        }
        break;

      case OPT_NUMTHR:
        m = static_cast<int>(strtol(optarg, &ctmp, 0));
        if (m < 0)
        {
          cout << "Number of threads must be >= 0\n\n";
          nextToken -= 2;
          errFlag = true;
        }
        options.num_threads_ = m;
        break;

      case OPT_MEMORY:
        m = static_cast<int>(strtol(optarg, &ctmp, 0));
        if (m < 0)
        {
          cout << "Memory in MB must be >= 0\n\n";
          nextToken -= 2;
          errFlag = true;
        }
        options.memory_mb_ = m;
        break;

      case OPT_REPORT:
        options.report_slow_boards_ = true;
        break;

      default:
        cout << "Unknown option\n";
        errFlag = true;
        break;
    }
    if (errFlag)
      break;
  }

  if (errFlag || c == -1)
  {
    cout << "Error while parsing option '" << argv[nextToken] << "'\n";
    cout << "Invoke the program without arguments for help" << endl;
    exit(0);
  }
}

