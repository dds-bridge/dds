/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_ABSTATS_H
#define DDS_ABSTATS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#define AB_LATERTRICKS 3
#define AB_MAIN_LOOKUP 4
#define AB_SIDE_LOOKUP 5
#define AB_MOVE_LOOP 6



#define DDS_MAXDEPTH 49
#define DDS_LINE_LEN 20
#define DDS_AB_POS 7



class ABstats
{
  private:
    FILE * fp;
    char fname[DDS_LINE_LEN];
    char name[DDS_AB_POS][40];
    int counter[DDS_AB_POS][DDS_MAXDEPTH];
    int counterCum[DDS_AB_POS];
    int pcounterCum[DDS_AB_POS];
    int score[2][DDS_MAXDEPTH];
    int scoreCum[2];
    int pscoreCum[2];
    int nodes[DDS_MAXDEPTH];
    int nodesCum[DDS_MAXDEPTH];
    int allnodes,
                        allnodesCum;
    int iniDepth;

  public:
    ABstats();
    ~ABstats();
    void Reset();
    void ResetCum();
    void SetFile(char * fname);
    void SetName(int no, char * name);
    void IncrPos(int no, bool side, int depth);
    void IncrNode(int depth);
    int GetNodes();
    void PrintStats();
};

#endif
