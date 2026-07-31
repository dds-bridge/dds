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
#include <cstring>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "args.hpp"
#include "cst.hpp"

using std::cout;
using std::endl;
using std::setw;
using std::right;
using std::left;
using std::vector;
using std::string;


extern OptionsType options;

struct optEntry
{
  string shortName;
  string longName;
  unsigned numArgs;
};

#define DTEST_NUM_OPTIONS 7

enum DtestOpt
{
  OPT_FILE = 0,
  OPT_SOLVER = 1,
  OPT_NUMTHR = 2,
  OPT_MEMORY = 3,
  OPT_REPORT = 4,
  OPT_MAX = 5,
  OPT_MIN = 6
};

const optEntry optList[DTEST_NUM_OPTIONS] =
{
  {"f", "file", 1},
  {"s", "solver", 1},
  {"n", "numthr", 1},
  {"m", "memory", 1},
  {"r", "report", 0},
  {"", "max", 0},
  {"", "min", 0}
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
  string basename(base);
  const size_t l = basename.find_last_of("\\/");
  if (l != string::npos)
    basename.erase(0, l+1);

  cout <<
    "Usage: " << basename << " [options]\n\n" <<
    "-f, --file s       Input file, or the number n;\n" <<
    "                   '100' means hands/list100.txt under the current\n" <<
    "                   directory (or BUILD_WORKING_DIRECTORY /\n" <<
    "                   BUILD_WORKSPACE_DIRECTORY under bazel run), else\n" <<
    "                   relative to the dtest binary\n" <<
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
    "-r, --report       Print per-board timings sorted by longest first.\n" <<
    "\n" <<
    "    --max          Also print max per-hand user/sys time across batches.\n" <<
    "\n" <<
    "    --min          Also print min per-hand user/sys time across batches.\n" <<
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

      // Return 1-based option index so --max/--min do not collide with
      // --memory on the shared leading 'm'.
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
  options.show_min_ = false;
  options.show_max_ = false;
}


namespace
{

bool is_absolute_path(const string& path)
{
  if (path.empty())
    return false;
#ifdef _WIN32
  return path.size() >= 2 && std::isalpha(static_cast<unsigned char>(path[0])) &&
    path[1] == ':';
#else
  return path[0] == '/' || path[0] == '\\';
#endif
}


// Collapse "." / ".." path segments without resolving symlinks.
string normalize_logical_path(const string& path)
{
  if (path.empty())
    return path;

  const bool absolute = is_absolute_path(path);
  string drive;
  size_t start = 0;
#ifdef _WIN32
  if (absolute && path.size() >= 2 && path[1] == ':')
  {
    drive = path.substr(0, 2);
    start = 2;
    if (start < path.size() && (path[start] == '/' || path[start] == '\\'))
      ++start;
  }
#else
  if (absolute)
    start = 1;
#endif

  vector<string> parts;
  string cur;
  auto flush = [&]()
  {
    if (cur.empty())
      return;
    if (cur == ".")
    {
      cur.clear();
      return;
    }
    if (cur == "..")
    {
      if (!parts.empty())
        parts.pop_back();
      else if (!absolute)
        parts.push_back("..");
      cur.clear();
      return;
    }
    parts.push_back(cur);
    cur.clear();
  };

  for (size_t i = start; i < path.size(); ++i)
  {
    const char c = path[i];
    if (c == '/' || c == '\\')
      flush();
    else
      cur.push_back(c);
  }
  flush();

  string out = drive;
  if (absolute)
#ifdef _WIN32
    out += '\\';
#else
    out += '/';
#endif

  for (size_t i = 0; i < parts.size(); ++i)
  {
    if (i > 0)
      out += '/';
    out += parts[i];
  }
  if (absolute && parts.empty())
    return out.empty() ? string("/") : out;
  return out.empty() ? string(".") : out;
}


// Logical absolute path for argv0 without resolving symlinks (so climbing out
// of bazel-bin still lands on the workspace, not the execroot).
string absolute_path_logical(const string& path)
{
  if (is_absolute_path(path))
    return normalize_logical_path(path);

  char cwd[4096];
#ifdef _WIN32
  if (_getcwd(cwd, static_cast<int>(sizeof(cwd))) == nullptr)
    return normalize_logical_path(path);
#else
  if (getcwd(cwd, sizeof(cwd)) == nullptr)
    return normalize_logical_path(path);
#endif
  if (path.empty())
    return normalize_logical_path(string(cwd));
  return normalize_logical_path(string(cwd) + "/" + path);
}

}  // namespace


string resolve_dtest_input_file(
  const string& arg,
  const string& argv0)
{
  struct stat buffer;
  if (stat(arg.c_str(), &buffer) == 0)
    return arg;

  const string list_name = "list" + arg + ".txt";
  const string cwd_candidate = "hands/" + list_name;
  if (stat(cwd_candidate.c_str(), &buffer) == 0)
    return cwd_candidate;

  // bazel run moves CWD into the runfiles tree; it exports the invoke-time
  // shell cwd and the workspace root so we can still find hands/.
  auto from_env_dir = [&](const char* env_name) -> string
  {
    const char* dir = std::getenv(env_name);
    if (dir == nullptr || dir[0] == '\0')
      return string();
    string base(dir);
    while (base.size() > 1 && (base.back() == '/' || base.back() == '\\'))
      base.pop_back();
    const string candidate = base + "/hands/" + list_name;
    if (stat(candidate.c_str(), &buffer) == 0)
      return candidate;
    return string();
  };

  if (const string found = from_env_dir("BUILD_WORKING_DIRECTORY"); !found.empty())
    return found;
  if (const string found = from_env_dir("BUILD_WORKSPACE_DIRECTORY"); !found.empty())
    return found;

  // Climb parents in the path *string* (do not use "/../" with stat — that
  // follows a bazel-bin symlink into the execroot and misses the workspace
  // hands/ directory). bazel-bin/library/tests → three levels up to repo root.
  const string abs_argv0 = absolute_path_logical(argv0);
  size_t slash = abs_argv0.find_last_of("\\/");
  if (slash == string::npos)
    return string();

  string dir = abs_argv0.substr(0, slash);
  for (unsigned i = 0; i < 3; ++i)
  {
    slash = dir.find_last_of("\\/");
    if (slash == string::npos)
      return string();
    dir = (slash == 0) ? dir.substr(0, 1) : dir.substr(0, slash);
  }

  const string bin_candidate = dir + "/hands/" + list_name;
  if (stat(bin_candidate.c_str(), &buffer) == 0)
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
        cout << "Also tried hands/list" << optarg <<
          ".txt under the current directory and relative to the "
          "dtest binary\n";
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

      case OPT_MAX:
        options.show_max_ = true;
        break;

      case OPT_MIN:
        options.show_min_ = true;
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

