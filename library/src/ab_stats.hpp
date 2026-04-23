/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <fstream>
#include <string>

#include <utility/debug.h>


/*
   AB_COUNT is a macro that avoids the tedious #ifdef's at
   the code places to be counted.
*/

#ifdef DDS_AB_STATS
  #define AB_COUNT(a, b, c) thrp->ABStats.IncrPos(a, b, c)
#else
  #define AB_COUNT(a, b, c)
#endif


enum ABCountType
{
  AB_TARGET_REACHED = 0,
  AB_DEPTH_ZERO = 1,
  AB_QUICKTRICKS = 2,
  AB_QUICKTRICKS_2ND = 3,
  AB_LATERTRICKS = 4,
  AB_MAIN_LOOKUP = 5,
  AB_SIDE_LOOKUP = 6,
  AB_MOVE_LOOP = 7,
  AB_SIZE = 8
};

#define DDS_MAXDEPTH 49


struct ABtracker
{
  int list[DDS_MAXDEPTH];
  int sum;
  int sumWeighted;
  int sumCum;
  int sumCumWeighted;
};


/**
 * @brief Alpha-beta search statistics accumulator for bridge double dummy solver.
 *
 * The ABstats class accumulates, tracks, and reports statistics related to
 * alpha-beta pruning and search operations during double dummy analysis. It
 * provides detailed breakdowns by position, depth, and side, supporting
 * performance tuning and debugging. Used internally for profiling.
 */
class ABstats
{
  private:

    std::string name[AB_SIZE];

    // A node arises when a new move is generated.
    // Not every move leads to an AB termination.
    ABtracker ABnodes;
    ABtracker ABnodesCum;

    // AB terminations are tracked by side and position.
    ABtracker ABsides[2];
    ABtracker ABplaces[AB_SIZE];

    void SetNames();

    void PrintHeaderPosition(std::ofstream& fout) const; 

    void PrintStatsPosition(
      std::ofstream& fout,
      const int no,
      const std::string& text,
      const ABtracker& abt,
      const ABtracker& divisor) const;

    void PrintHeaderDepth(std::ofstream& fout) const; 

    void PrintStatsDepth(
      std::ofstream& fout,
      const int depth,
      const int cum) const; 

    void PrintAverageDepth(
      std::ofstream& fout,
      const ABtracker& ABsidesSum) const; 

    void PrintHeaderDetail(std::ofstream& fout) const; 

    void PrintStatsDetail(
      std::ofstream& fout,
      const int depth) const; 

    void PrintSumDetail(std::ofstream& fout) const; 

  public:

    /**
     * @brief Construct a new ABstats object.
     *
     * Initializes the alpha-beta statistics accumulator.
     */
    ABstats();

    /**
     * @brief Destroy the ABstats object and clean up resources.
     *
     * Releases all memory and resets the statistics state.
     */
    ~ABstats();

    void Reset();

    void ResetCum();

    void IncrPos(
      const ABCountType no, 
      const bool side, 
      const int depth);

    void IncrNode(const int depth);

    int GetNodes() const;

    void PrintStats(std::ofstream& fout);
};

