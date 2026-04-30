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

#define DTEST_NUM_OPTIONS 5

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
  string basename(base);
  const size_t l = basename.find_last_of("\\/");
  if (l != string::npos)
    basename.erase(0, l+1);

  cout <<
    "Usage: " << basename << " [options]\n\n" <<
    "-f, --file s       Input file, or the number n;\n" <<
    "                   '100' means ../hands/list100.txt).\n" <<
    "                   (Default: input.txt)\n" <<
    "\n" <<
    "-s, --solver       One of: solve, calc, play, par, dealerpar.\n" <<
    "                   (Default: solve)\n" <<
    "\n" <<
    "-n, --numthr n     Maximum number of threads (legacy option).\n" <<
    "                   (Default: 0 uses DDS/library defaults; when using\n" <<
    "                   the modern SolverContext API, prefer configuring\n" <<
    "                   threads via SolverConfig instead of this option.)\n" <<
    "\n" <<
    "-m, --memory n     Total DDS memory size in MB (legacy option).\n" <<
    "                   (Default: 0 uses DDS/library defaults; when using\n" <<
    "                   the modern SolverContext API, prefer configuring\n" <<
    "                   memory via SolverConfig instead of this option.)\n" <<
    "\n" <<
    "-r, --report       Print per-board timings sorted by longest first.\n" <<
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
    if (str == optList[i].shortName || str == optList[i].longName)
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

      return str[0];
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
  struct stat buffer;

  while ((c = GetNextArgToken(argc, argv)) > 0)
  {
    switch(c)
    {
      case 'f':
        if (stat(optarg, &buffer) == 0)
        {
          options.fname_ = string(optarg);
          break;
        }

        stmp = "../hands/list" + string(optarg) + ".txt";
        if (stat(stmp.c_str(), &buffer) == 0)
        {
          options.fname_ = stmp;
          break;
        }

        cout << "Input file '" << optarg << "' not found\n";
        cout << "Input file '" << stmp << "' not found\n";
        nextToken -= 2;
        errFlag = true;
        break;

      case 's':
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

      case 'n':
        m = static_cast<int>(strtol(optarg, &ctmp, 0));
        if (m < 0)
        {
          cout << "Number of threads must be >= 0\n\n";
          nextToken -= 2;
          errFlag = true;
        }
        options.num_threads_ = m;
        break;

      case 'm':
        m = static_cast<int>(strtol(optarg, &ctmp, 0));
        if (m < 0)
        {
          cout << "Memory in MB must be >= 0\n\n";
          nextToken -= 2;
          errFlag = true;
        }
        options.memory_mb_ = m;
        break;

      case 'r':
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

