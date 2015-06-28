/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2015 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_TRANSTABLES_H
#define DDS_TRANSTABLES_H

/*
   This is an object for managing transposition tables and the
   associated memory.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dds.h"
#include "../include/dll.h"


#define NUM_PAGES_DEFAULT 15
#define NUM_PAGES_MAXIMUM 25
#define BLOCKS_PER_PAGE 1000
#define DISTS_PER_ENTRY 32
#define BLOCKS_PER_ENTRY 125
#define FIRST_HARVEST_TRICK 8
#define HARVEST_AGE 10000

#define TT_BYTES 4
#define TT_TRICKS 12

#define TT_LINE_LEN 20

#define TT_PERCENTILE 0.9

#define HISTSIZE 100000


// Also used in ABSearch
struct nodeCardsType // 8 bytes
{
  char ubound; // For N-S
  char lbound; // For N-S
  char bestMoveSuit;
  char bestMoveRank;
  char leastWin[DDS_SUITS];
};


class TransTable
{
  private:

    struct winMatchType // 52 bytes
    {
      unsigned xorSet;
      unsigned topSet1 , topSet2 , topSet3 , topSet4 ;
      unsigned topMask1, topMask2, topMask3, topMask4;
      int maskIndex;
      int lastMaskNo;
      nodeCardsType first;
    };

    struct winBlockType // 6508 bytes when BLOCKS_PER_ENTRY == 125
    {
      int nextMatchNo;
      int nextWriteNo;
      // int timestampWrite;
      int timestampRead;
      winMatchType list[BLOCKS_PER_ENTRY];
    };

    struct posSearchType // 16 bytes (inefficiency, 12 bytes enough)
    {
      winBlockType * posBlock;
      long long key;
    };

    struct distHashType // 520 bytes when DISTS_PER_ENTRY == 32
    {
      int nextNo;
      int nextWriteNo;
      posSearchType list[DISTS_PER_ENTRY];
    };

    struct aggrType // 80 bytes
    {
      unsigned aggrRanks[DDS_SUITS];
      unsigned aggrBytes[DDS_SUITS][TT_BYTES];
    };

    struct poolType // 16 bytes
    {
      poolType * next;
      poolType * prev;
      int nextBlockNo;
      winBlockType * list;
    };

    struct pageStatsType
    {
      int numResets,
                        numCallocs,
                        numFrees,
                        numHarvests,
                        lastCurrent;
    };

    struct harvestedType // 16 bytes
    {
      int nextBlockNo;
      winBlockType * list [BLOCKS_PER_PAGE];
    };

    enum memStateType
    {
      FROM_POOL,
      FROM_HARVEST
    };

    memStateType memState;

    int timestamp;

    int pagesDefault,
                        pagesCurrent,
                        pagesMaximum;

    int harvestTrick,
                        harvestHand;

    pageStatsType pageStats;


    // aggr is constant for a given hand.
    aggrType aggr[8192]; // 64 KB

    // This is the real transposition table.
    // The last index is the hash.
    // 6240 KB with above assumptions
    // distHashType TTroot[TT_TRICKS][DDS_HANDS][256];
    distHashType * TTroot[TT_TRICKS][DDS_HANDS];

    int TTInUse;

    // It is useful to remember the last block we looked at.
    winBlockType * lastBlockSeen[TT_TRICKS][DDS_HANDS];

    // The pool of card entries for a given suit distribution.
    poolType * poolp;
    winBlockType * nextBlockp;
    harvestedType harvested;


    void InitTT();

    void ReleaseTT();

    void SetConstants();

    int hash8(int * handDist);

    // int BlocksInUse();

    winBlockType * GetNextCardBlock();

    winBlockType * LookupSuit(
      distHashType * dp,
      long long key,
      bool * empty);


    nodeCardsType * LookupCards(
      winMatchType * searchp,
      winBlockType * bp,
      int limit,
      bool * lowerFlag);

    void CreateOrUpdate(
      winBlockType * bp,
      winMatchType * searchp,
      bool flag);

    bool Harvest();

    // Debug

    FILE * fp;

    char fname[TT_LINE_LEN];

    // Really the maximum of BLOCKS_PER_ENTRY and DISTS_PER_ENTRY
    int suitHist[BLOCKS_PER_ENTRY + 1],
                        suitWraps;

    void KeyToDist(
      long long key,
      int handDist[]);

    void DistToLengths(
      int trick,
      int handDist[],
      unsigned char lengths[DDS_HANDS][DDS_SUITS]);

    void LenToStr(
      unsigned char lengths[DDS_HANDS][DDS_SUITS],
      char * line);

    void MakeHistStats(
      int hist[],
      int * count,
      int * prod_sum,
      int * prod_sumsq,
      int * max_len,
      int last_index);

    int CalcPercentile(
      int hist[],
      double threshold,
      int last_index);

    void PrintHist(
      int hist[],
      int num_wraps,
      int last_index);

    void UpdateSuitHist(
      int trick,
      int hand,
      int hist[],
      int * num_wraps);

    winBlockType * FindMatchingDist(
      int trick,
      int hand,
      int handDistSought[DDS_HANDS]);

    void PrintEntriesBlock(
      winBlockType * bp,
      unsigned char lengths[DDS_HANDS][DDS_SUITS]);

    void UpdateEntryHist(
      int trick,
      int hand,
      int hist[],
      int * num_wraps);

    int EffectOfBlockBound(
      int hist[],
      int size);

    void PrintNodeValues(
      nodeCardsType * np);

    void PrintMatch(
      winMatchType * wp,
      unsigned char lengths[DDS_HANDS][DDS_SUITS]);

    void MakeHolding(
      char * high,
      unsigned len,
      char * res);

    void DumpHands(
      char hands[DDS_SUITS][DDS_HANDS][TT_LINE_LEN],
      unsigned char lengths[DDS_HANDS][DDS_SUITS]);

    void SetToPartialHands(
      unsigned set,
      unsigned mask,
      int maxRank,
      int numRanks,
      char hands[DDS_SUITS][DDS_HANDS][TT_LINE_LEN],
      int used[DDS_SUITS][DDS_HANDS]);


  public:
    TransTable();

    ~TransTable();

    void Init(int handLookup[][15]);

    void SetMemoryDefault(int megabytes);

    void SetMemoryMaximum(int megabytes);

    void MakeTT();

    void ResetMemory();

    void ReturnAllMemory();

    double MemoryInUse();

    void Top4Ranks(
      unsigned short aggrTarget[],
      unsigned rr[DDS_SUITS]);

    nodeCardsType * Lookup(
      int trick,
      int hand,
      unsigned short * aggrTarget,
      int * handDist,
      int limit,
      bool * lowerFlag);

    void Add(
      int trick,
      int hand,
      unsigned short * aggrTarget,
      unsigned short * winRanks,
      nodeCardsType * first,
      bool flag);

    // Debug functions

    void SetFile(
      char * fname);

    void PrintSuits(
      int trick,
      int hand);

    void PrintAllSuits();

    void PrintSuitStats(
      int trick,
      int hand);

    void PrintAllSuitStats();

    void PrintSummarySuitStats();

    // Examples:
    // int hd[DDS_HANDS] = { 0x0342, 0x0334, 0x0232, 0x0531 };
    // thrp->transTable.PrintEntriesDist(11, 1, hd);
    // unsigned short ag[DDS_HANDS] =
    // { 0x1fff, 0x1fff, 0x0f75, 0x1fff };
    // thrp->transTable.PrintEntriesDistAndCards(11, 1, ag, hd);

    void PrintEntriesDist(
      int trick,
      int hand,
      int handDist[DDS_HANDS]);

    void PrintEntriesDistAndCards(
      int trick,
      int hand,
      unsigned short * aggrTarget,
      int handDist[DDS_HANDS]);

    void PrintEntries(
      int trick,
      int hand);

    void PrintAllEntries();

    void PrintEntryStats(
      int trick,
      int hand);

    void PrintAllEntryStats();

    void PrintSummaryEntryStats();

    void PrintPageSummary();


    // Could also be made private, see above.
    int BlocksInUse();
};

#endif
