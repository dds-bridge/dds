/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_ABSTATS_H
#define DDS_ABSTATS_H

#include <iostream>
#include <fstream>
#include <string>

#include "debug.h"

using namespace std;


/*
   AB_COUNT is a macro that avoids the tedious #ifdef's at
   the code places to be counted.
*/

#ifdef DDS_AB_STATS
  #define AB_COUNT(a, b, c) thrp->ABStats.IncrPos(a, b, c)
#else
  #define AB_COUNT(a, b, c) 1
#endif


#define AB_TARGET_REACHED 0
#define AB_DEPTH_ZERO 1
#define AB_QUICKTRICKS 2
#define AB_QUICKTRICKS_2ND 3
#define AB_LATERTRICKS 4
#define AB_MAIN_LOOKUP 5
#define AB_SIDE_LOOKUP 6
#define AB_MOVE_LOOP 7

#define DDS_AB_POS 8

#define DDS_MAXDEPTH 49


struct ABtracker
{
  int list[DDS_MAXDEPTH];
  int sum;
  int sumWeighted;
  int sumCum;
  int sumCumWeighted;
};


class ABstats
{
  private:

    string fname;
    string name[DDS_AB_POS];

    // A node arises when a new move is generated.
    // Not every move leads to an AB termination.
    ABtracker ABnodes;
    ABtracker ABnodesCum;

    // AB terminations are tracked by side and position.
    ABtracker ABsides[2];
    ABtracker ABplaces[DDS_AB_POS];

    void PrintHeaderPosition(ofstream& fout) const; 

    void PrintStatsPosition(
      ofstream& fout,
      const int no,
      const string& text,
      const ABtracker& abt,
      const ABtracker& divisor) const;

    void PrintHeaderDepth(ofstream& fout) const; 

    void PrintStatsDepth(
      ofstream& fout,
      const int depth,
      const int cum) const; 

    void PrintAverageDepth(
      ofstream& fout,
      const ABtracker& ABsidesSum) const; 

    void PrintHeaderDetail(ofstream& fout) const; 

    void PrintStatsDetail(
      ofstream& fout,
      const int depth) const; 

    void PrintSumDetail(ofstream& fout) const; 

  public:

    ABstats();

    ~ABstats();

    void Reset();

    void ResetCum();

    void SetFile(const string& fnameIn);

    void SetName(
      const int no, 
      const string& nameIn);

    void IncrPos(
      const int no, 
      const bool side, 
      const int depth);

    void IncrNode(const int depth);

    int GetNodes() const;

    void PrintStats();
};

#endif
