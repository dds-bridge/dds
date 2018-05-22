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
      const pos& tpos,
      const moveType& bestMove,
      const moveType& bestMoveTT,
      const relRanksType thrp_rel[]);

    void WeightAllocNT0(
      const pos& tpos,
      const moveType& bestMove,
      const moveType& bestMoveTT,
      const relRanksType thrp_rel[]);

    void WeightAllocTrumpNotvoid1( const pos& tpos);
    void WeightAllocNTNotvoid1(const pos& tpos);
    void WeightAllocTrumpVoid1(const pos& tpos);
    void WeightAllocNTVoid1(const pos& tpos);
    void WeightAllocTrumpNotvoid2(const pos& tpos);
    void WeightAllocNTNotvoid2(const pos& tpos);
    void WeightAllocTrumpVoid2(const pos& tpos);
    void WeightAllocNTVoid2(const pos& tpos);
    void WeightAllocCombinedNotvoid3(const pos& tpos);
    void WeightAllocTrumpVoid3(const pos& tpos);
    void WeightAllocNTVoid3(const pos& tpos);

    void GetTopNumber(
      const int ris,
      const int prank,
      int& topNumber,
      int& mno) const;

    int RankForcesAce(int cards4th) const;

    typedef void (Moves::*WeightPtr)(const pos& tpos);
    WeightPtr WeightList[16];

    inline bool WinningMove(
      const moveType& mvp1,
      const extCard& mvp2,
      const int trump) const;

    string PrintMove(const movePlyType& mply) const;

    void MergeSort();

    void UpdateStatsEntry(
      moveStatsType& stat,
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
      const unsigned short rankInSuit[DDS_HANDS][DDS_SUITS],
      const int trump,
      const int leadHand);

    void Reinit(
      const int tricks,
      const int leadHand);

    int MoveGen0(
      const int tricks,
      const pos& tpos,
      const moveType& bestMove,
      const moveType& bestMoveTT,
      const relRanksType thrp_rel[]);

    int MoveGen123(
      const int tricks,
      const int relHand,
      const pos& tpos);

    int GetLength(
      const int trick,
      const int relHand) const;

    void MakeSpecific(
      const moveType& mply,
      const int trick,
      const int relHand);

    moveType const * MakeNext(
      const int trick,
      const int relHand,
      const unsigned short winRanks[DDS_SUITS]);

    moveType const * MakeNextSimple(
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

    const trickDataType& GetTrickData(const int tricks);

    void Sort(
      const int tricks,
      const int relHand);

    string PrintMoves(
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
