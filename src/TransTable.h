/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

/*
   This is the parent class of TransTableS and TransTableL.
   Those two are different implementations.  The S version has a
   much smaller memory and a somewhat slower execution time.
*/

#ifndef DDS_TRANSTABLE_H
#define DDS_TRANSTABLE_H

#include <iostream>
#include <fstream>
#include <string>

#include "dds.h"

using namespace std;


enum TTresetReason
{
  TT_RESET_UNKNOWN = 0,
  TT_RESET_TOO_MANY_NODES = 1,
  TT_RESET_NEW_DEAL = 2,
  TT_RESET_NEW_TRUMP = 3,
  TT_RESET_MEMORY_EXHAUSTED = 4,
  TT_RESET_FREE_MEMORY = 5,
  TT_RESET_SIZE = 6
};

struct nodeCardsType // 8 bytes
{
  char ubound; // For N-S
  char lbound; // For N-S
  char bestMoveSuit;
  char bestMoveRank;
  char leastWin[DDS_SUITS];
};

#ifdef _MSC_VER
  // Disable warning for unused arguments.
  #pragma warning(push)
  #pragma warning(disable: 4100)
#endif

#ifdef __APPLE__
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

class TransTable
{
  public:
    TransTable(){};

    virtual ~TransTable(){};

    virtual void Init(const int handLookup[][15]){};

    virtual void SetMemoryDefault(const int megabytes){};

    virtual void SetMemoryMaximum(const int megabytes){};

    virtual void MakeTT(){};

    virtual void ResetMemory(const TTresetReason reason){};

    virtual void ReturnAllMemory(){};

    virtual double MemoryInUse() const {return 0.;};

    virtual nodeCardsType const * Lookup(
      const int trick,
      const int hand,
      const unsigned short aggrTarget[],
      const int handDist[],
      const int limit,
      bool& lowerFlag){return NULL;};

    virtual void Add(
      const int trick,
      const int hand,
      const unsigned short aggrTarget[],
      const unsigned short winRanksArg[],
      const nodeCardsType& first,
      const bool flag){};

    virtual void PrintSuits(
      ofstream& fout, 
      const int trick, 
      const int hand) const {};

    virtual void PrintAllSuits(ofstream& fout) const {};

    virtual void PrintSuitStats(
      ofstream& fout, 
      const int trick, 
      const int hand) const {};

    virtual void PrintAllSuitStats(ofstream& fout) const {};

    virtual void PrintSummarySuitStats(ofstream& fout) const {};

    virtual void PrintEntriesDist(
      ofstream& fout, 
      const int trick,
      const int hand,
      const int handDist[]) const {};

    virtual void PrintEntriesDistAndCards(
      ofstream& fout,
      const int trick,
      const int hand,
      const unsigned short aggrTarget[],
      const int handDist[]) const {};

    virtual void PrintEntries(
      ofstream& fout, 
      const int trick, 
      const int hand) const {};

    virtual void PrintAllEntries(ofstream& fout) const {};

    virtual void PrintEntryStats(
      ofstream& fout, 
      const int trick, 
      const int hand) const {};

    virtual void PrintAllEntryStats(ofstream& fout) const {};

    virtual void PrintSummaryEntryStats(ofstream& fout) const {};

    virtual void PrintPageSummary(ofstream& fout) const {};

    virtual void PrintNodeStats(ofstream& fout) const {};

    virtual void PrintResetStats(ofstream& fout) const {};
};

#ifdef _MSC_VER
  #pragma warning(pop)
#endif

#ifdef __APPLE__
  #pragma clang diagnostic pop
#endif

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif

#endif
