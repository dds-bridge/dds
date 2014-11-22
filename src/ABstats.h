/*
   DDS 2.7.0   A bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund

   Cleanups and porting to Linux and MacOSX (C) 2006 by Alex Martelli.

   The code for calculation of par score / contracts is based upon the

   perl code written by Matthew Kidd for ACBLmerge. He has kindly given
 
   me permission to include a C++ adaptation in DDS. 
*/



/*
   Licensed under the Apache License, Version 2.0 (the "License");

   you may not use this file except in compliance with the License.

   You may obtain a copy of the License at


   http://www.apache.org/licenses/LICENSE-2.0


   Unless required by applicable law or agreed to in writing, software

   distributed under the License is distributed on an "AS IS" BASIS,

   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 
   implied.  See the License for the specific language governing
 
   permissions and limitations under the License.
*/



/* 
   This object, ABstats, was was written by Søren Hein.
 
   Many thanks for allowing me to include it in DDS.
 

   It is a simple object for AB statistics and return values.

   As it stands, it is quite specific to AB counts.
*/




#ifndef _DDS_AB_STATS

#define _DDS_AB_STATS


#include <stdio.h>

#include <stdlib.h>

#include <string.h>



/*
   AB_COUNT is a macro that avoids the tedious #ifdef's at

   the code places to be counted.
*/



#ifdef DDS_AB_STATS

#define AB_COUNT(a, b, c)
 thrp->ABstats.IncrPos(a, b, c)

#else

#define AB_COUNT(a, b, c) 1

#endif



#define AB_TARGET_REACHED	0

#define AB_DEPTH_ZERO		1

#define AB_QUICKTRICKS		2

#define AB_LATERTRICKS		3

#define AB_MAIN_LOOKUP		4

#define AB_SIDE_LOOKUP		5

#define AB_MOVE_LOOP		6




#define DDS_MAXDEPTH	49

#define DDS_LINE_LEN	20

#define DDS_AB_POS	 7





class ABstats

{

  private:

    FILE * fp;

    char fname[DDS_LINE_LEN];

    char name[DDS_AB_POS][40];

    int	counter[DDS_AB_POS][DDS_MAXDEPTH];

    int	counterCum[DDS_AB_POS],
 pcounterCum[DDS_AB_POS];

    int	score[2][DDS_MAXDEPTH];

    int	scoreCum[2],
 pscoreCum[2];

    int	nodes[DDS_MAXDEPTH],
 nodesCum[DDS_MAXDEPTH];

    int	allnodes,
 allnodesCum;

    int	iniDepth;



  public:

    ABstats();

    ~ABstats();

    void Reset();

    void ResetCum();

    void SetFile(char * fname);

    void SetName(int no, char * name);

    void IncrPos(int no, bool side, int depth);

    void IncrNode(int depth);

    int	GetNodes();

    void PrintStats();

};


#endif
