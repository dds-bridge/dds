/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_MOVES_H
#define DDS_MOVES_H

#include <iostream>
#include <fstream>
#include <string>

#include "dds.h"
#include "../include/dll.h"

using namespace std;


enum MGtype
{
  MG_NT0 = 0,
  MG_TRUMP0 = 1,
  MG_NT_VOID1 = 2,
  MG_TRUMP_VOID1 = 3,
  MG_NT_NOTVOID1 = 4,
  MG_TRUMP_NOTVOID1 = 5,
  MG_NT_VOID2 = 6,
  MG_TRUMP_VOID2 = 7,
  MG_NT_NOTVOID2 = 8,
  MG_TRUMP_NOTVOID2 = 9,
  MG_NT_VOID3 = 10,
  MG_TRUMP_VOID3 = 11,
  MG_COMB_NOTVOID3 = 12,
  MG_SIZE = 13
};


struct trickDataType
{
  int playCount[DDS_SUITS];
  int bestRank;
  int bestSuit;
  int bestSequence;
  int relWinner;
  int nextLeadHand;
};


class Moves
{
  private:

    int leadHand;
    int leadSuit;
    int currHand;
    int currSuit;
    int currTrick;
    int trump;
    int suit;
    int numMoves;
    int lastNumMoves;

    struct trackType
    {
      int leadHand;
      int leadSuit;
      int playSuits[DDS_HANDS];
      int playRanks[DDS_HANDS];
      trickDataType trickData;
      extCard move[DDS_HANDS];
      int high[DDS_HANDS];
      int lowestWin[DDS_HANDS][DDS_SUITS];
      int removedRanks[DDS_SUITS];
    };

    trackType track[13];
    trackType * trackp;

    movePlyType moveList[13][DDS_HANDS];

    moveType * mply;

    MGtype lastCall[13][DDS_HANDS];

    string funcName[MG_SIZE];

    struct moveStatType
    {
      int count;
      int findex;
      int sumHits;
      int sumLengths;
    };

    struct moveStatsType
    {
      int nfuncs;
      moveStatType list[MG_SIZE];
    };

    moveStatType trickTable[13][DDS_HANDS];

    moveStatType trickSuitTable[13][DDS_HANDS];

    moveStatsType trickDetailTable[13][DDS_HANDS];

    moveStatsType trickDetailSuitTable[13][DDS_HANDS];

    moveStatsType trickFuncTable;

    moveStatsType trickFuncSuitTable;


    void WeightAllocTrump0(
      pos const * posPoint,
      const moveType& bestMove,
      const moveType& bestMoveTT,
      const relRanksType thrp_rel[]);

    void WeightAllocNT0(
      pos const * posPoint,
      const moveType& bestMove,
      const moveType& bestMoveTT,
      const relRanksType thrp_rel[]);

    void WeightAllocTrumpNotvoid1(
      pos const * posPoint);

    void WeightAllocNTNotvoid1(pos const * posPoint);
    void WeightAllocTrumpVoid1(pos const * posPoint);
    void WeightAllocNTVoid1(pos const * posPoint);
    void WeightAllocTrumpNotvoid2(pos const * posPoint);
    void WeightAllocNTNotvoid2(pos const * posPoint);
    void WeightAllocTrumpVoid2(pos const * posPoint);
    void WeightAllocNTVoid2(pos const * posPoint);
    void WeightAllocCombinedNotvoid3(pos const * posPoint);
    void WeightAllocTrumpVoid3(pos const * posPoint);
    void WeightAllocNTVoid3(pos const * posPoint);

    void GetTopNumber(
      const int ris,
      const int prank,
      int& topNumber,
      int& mno) const;

    int RankForcesAce(int cards4th) const;

    typedef void (Moves::*WeightPtr)(pos const * posPoint);
    WeightPtr WeightList[16];

    inline bool WinningMove(
      const moveType& mvp1,
      const extCard& mvp2,
      const int trump) const;

    void PrintMove(
      movePlyType const * mply) const;

    void MergeSort();

    void UpdateStatsEntry(
      moveStatsType * statp,
      const int findex,
      const int hit,
      const int len) const;

    string AverageString(const moveStatType& statp) const;

    string FullAverageString(const moveStatType& statp) const;

    string PrintTrickTable(const moveStatType tablep[][DDS_HANDS]) const;

    string PrintFunctionTable(const moveStatsType& tablep) const;

  public:
    Moves();

    ~Moves();

    void Init(
      const int tricks,
      const int relStartHand,
      const int initialRanks[],
      const int initialSuits[],
      const unsigned short int rankInSuit[DDS_HANDS][DDS_SUITS],
      const int trump,
      const int leadHand);

    void Reinit(
      const int tricks,
      const int leadHand);

    int MoveGen0(
      const int tricks,
      pos const * posPoint,
      const moveType& bestMove,
      const moveType& bestMoveTT,
      const relRanksType thrp_rel[]);

    int MoveGen123(
      const int tricks,
      const int relHand,
      pos const * posPoint);

    int GetLength(
      const int trick,
      const int relHand) const;

    void MakeSpecific(
      moveType const * mply,
      const int trick,
      const int relHand);

    moveType * MakeNext(
      const int trick,
      const int relHand,
      const unsigned short winRanks[DDS_SUITS]);

    moveType * MakeNextSimple(
      const int trick,
      const int relHand);

    void Step(
      const int tricks,
      const int relHand);

    void Rewind(
      const int tricks,
      const int relHand);

    void Purge(
      const int tricks,
      const int relHand,
      const moveType forbiddenMoves[]);

    void Reward(
      const int trick,
      const int relHand);

    trickDataType * GetTrickData(
      const int tricks);

    void Sort(
      const int tricks,
      const int relHand);

    void PrintMoves(
      const int trick,
      const int relHand) const;

    void RegisterHit(
      const int tricks,
      const int relHand);

    string TrickToText(const int trick) const;

    void PrintTrickStats(ofstream& fout) const;

    void PrintTrickDetails(ofstream& fout) const;

    void PrintFunctionStats(ofstream& fout) const;
};

#endif
