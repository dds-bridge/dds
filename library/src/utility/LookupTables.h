/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_UTILITY_LOOKUPTABLES_H
#define DDS_UTILITY_LOOKUPTABLES_H

// Definition of moveGroupType
struct moveGroupType
{
  // There are at most 7 groups of bit "runs" in a 13-bit vector
  int lastGroup;
  int rank[7];
  int sequence[7];
  int fullseq[7];
  int gap[7];
};

// Lookup table arrays
extern int highestRank[8192];
extern int lowestRank[8192];
extern int counttable[8192];
extern char relRank[8192][15];
extern unsigned short int winRanks[8192][14];
extern moveGroupType groupData[8192];

// Initialization function
void InitLookupTables();

#endif
