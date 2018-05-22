/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_TRANSTABLEL_H
#define DDS_TRANSTABLEL_H

/*
   This is an implementation of the transposition table that requires
   a lot of memory and is somewhat faster than the small version.
*/


#include <vector>
#include <string>

#include "../include/dll.h"
#include "dds.h"

#include "TransTable.h"

using namespace std;


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


class TransTableL: public TransTable
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
      int numResets;
      int numCallocs;
      int numFrees;
      int numHarvests;
      int lastCurrent;
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

    // Private data for the full memory version.
    memStateType memState;

    int pagesDefault;
    int pagesCurrent;
    int pagesMaximum;

    int harvestTrick;
    int harvestHand;

    pageStatsType pageStats;

    // aggr is constant for a given hand.
    aggrType aggr[8192]; // 64 KB

    // This is the real transposition table.
    // The last index is the hash.
    // 6240 KB with above assumptions
    // distHashType TTroot[TT_TRICKS][DDS_HANDS][256];
    distHashType * TTroot[TT_TRICKS][DDS_HANDS];

    // It is useful to remember the last block we looked at.
    winBlockType * lastBlockSeen[TT_TRICKS][DDS_HANDS];

    // The pool of card entries for a given suit distribution.
    poolType * poolp;
    winBlockType * nextBlockp;
    harvestedType harvested;

    int timestamp;
    int TTInUse;


    void InitTT();

    void ReleaseTT();

    void SetConstants();

    int hash8(const int handDist[]) const;

    winBlockType * GetNextCardBlock();

    winBlockType * LookupSuit(
      distHashType * dp,
      const long long key,
      bool& empty);

    nodeCardsType * LookupCards(
      const winMatchType& search,
      winBlockType * bp,
      const int limit,
      bool& lowerFlag);

    void CreateOrUpdate(
      winBlockType * bp,
      const winMatchType& search,
      const bool flag);

    bool Harvest();

    // Debug functions from here on.

    void KeyToDist(
      const long long key,
      int handDist[]) const;

    void DistToLengths(
      const int trick,
      const int handDist[],
      unsigned char lengths[DDS_HANDS][DDS_SUITS]) const;

    string SingleLenToStr(const unsigned char length[]) const;

    string LenToStr(
      const unsigned char lengths[DDS_HANDS][DDS_SUITS]) const;

    void MakeHistStats(
      const int hist[],
      int& count,
      int& prod_sum,
      int& prod_sumsq,
      int& max_len,
      const int last_index) const;

    int CalcPercentile(
      const int hist[],
      const double threshold,
      const int last_index) const;

    void PrintHist(
      ofstream& fout,
      const int hist[],
      const int num_wraps,
      const int last_index) const;

    void UpdateSuitHist(
      const int trick,
      const int hand,
      int hist[],
      int& num_wraps) const;

    void UpdateSuitHist(
      const int trick,
      const int hand,
      int hist[],
      int suitHist[],
      int& num_wraps,
      int& suitWraps) const;

    winBlockType const * FindMatchingDist(
      const int trick,
      const int hand,
      const int handDistSought[]) const;

    void PrintEntriesBlock(
      ofstream& fout,
      winBlockType const * bp,
      const unsigned char lengths[DDS_HANDS][DDS_SUITS]) const;

    void UpdateEntryHist(
      const int trick,
      const int hand,
      int hist[],
      int& num_wraps) const;

    void UpdateEntryHist(
      const int trick,
      const int hand,
      int hist[],
      int suitHist[],
      int& num_wraps,
      int& suitWraps) const;

    int EffectOfBlockBound(
      const int hist[],
      const int size) const;

    void PrintNodeValues(
      ofstream& fout,
      const nodeCardsType& np) const;

    void PrintMatch(
      ofstream& fout,
      const winMatchType& wp,
      const unsigned char lengths[DDS_HANDS][DDS_SUITS]) const;

    string MakeHolding(
      const string& high,
      const unsigned len) const;

    void DumpHands(
      ofstream& fout,
      const vector<vector<string>>& hands,
      const unsigned char lengths[DDS_HANDS][DDS_SUITS]) const;

    void SetToPartialHands(
      const unsigned set,
      const unsigned mask,
      const int maxRank,
      const int numRanks,
      vector<vector<string>>& hands) const;

    int BlocksInUse() const;

  public:
    TransTableL();

    ~TransTableL();

    void Init(const int handLookup[][15]);

    void SetMemoryDefault(const int megabytes);

    void SetMemoryMaximum(const int megabytes);

    void MakeTT();

    void ResetMemory(const TTresetReason reason);

    void ReturnAllMemory();

    double MemoryInUse() const;

    nodeCardsType * Lookup(
      const int trick,
      const int hand,
      const unsigned short aggrTarget[],
      const int handDist[],
      const int limit,
      bool& lowerFlag);

    void Add(
      const int trick,
      const int hand,
      const unsigned short aggrTarget[],
      const unsigned short winRanksArg[],
      const nodeCardsType& first,
      const bool flag);

    void PrintSuits(
      ofstream& fout,
      const int trick,
      const int hand) const;

    void PrintAllSuits(ofstream& fout) const;

    void PrintSuitStats(
      ofstream& fout,
      const int trick,
      const int hand) const;

    void PrintAllSuitStats(ofstream& fout) const;

    void PrintSummarySuitStats(ofstream& fout) const;

    // Examples:
    // int hd[DDS_HANDS] = { 0x0342, 0x0334, 0x0232, 0x0531 };
    // thrp->transTable.PrintEntriesDist(cout, 11, 1, hd);
    // unsigned short ag[DDS_HANDS] =
    // { 0x1fff, 0x1fff, 0x0f75, 0x1fff };
    // thrp->transTable.PrintEntriesDistAndCards(cout, 11, 1, ag, hd);

    void PrintEntriesDist(
      ofstream& fout,
      const int trick,
      const int hand,
      const int handDist[]) const;

    void PrintEntriesDistAndCards(
      ofstream& fout,
      const int trick,
      const int hand,
      const unsigned short aggrTarget[],
      const int handDist[]) const;

    void PrintEntries(
      ofstream& fout,
      const int trick,
      const int hand) const;

    void PrintAllEntries(ofstream& fout) const;

    void PrintEntryStats(
      ofstream& fout,
      const int trick,
      const int hand) const;

    void PrintAllEntryStats(ofstream& fout) const;

    void PrintSummaryEntryStats(ofstream& fout) const;
};

#endif
