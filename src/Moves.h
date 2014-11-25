/* 
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund / 
   2014 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#ifndef _DDS_MOVES
#define _DDS_MOVES

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dds.h"
#include "../include/dll.h"
#include "Stats.h"

#define CMP_SWAP(i, j) if (a[i].weight < a[j].weight) \
  { moveType tmp = a[i]; a[i] = a[j]; a[j] = tmp; }

#define CMP_SWAP_NEW(i, j) if (mply[i].weight < mply[j].weight) \
  { tmp = mply[i]; mply[i] = mply[j]; mply[j] = tmp; }

#define MG_NT0                  0
#define MG_TRUMP0               1
#define MG_NT_VOID1             2
#define MG_TRUMP_VOID1          3
#define MG_NT_NOTVOID1          4
#define MG_TRUMP_NOTVOID1       5
#define MG_NT_VOID2             6
#define MG_TRUMP_VOID2          7
#define MG_NT_NOTVOID2          8
#define MG_TRUMP_NOTVOID2       9
#define MG_NT_VOID3             10
#define MG_TRUMP_VOID3          11
#define MG_COMB_NOTVOID3        12
#define MG_NUM_FUNCTIONS        13


struct trickDataType {
  int                   playCount[DDS_SUITS];
  int                   bestRank,
                        bestSuit,
                        bestSequence,
                        relWinner,
                        nextLeadHand;
};


class Moves
{
  private:

    int                 leadHand,
                        leadSuit,
                        currHand,
                        currSuit,
                        currTrick,
                        trump,
                        suit,
                        numMoves,
                        lastNumMoves;

    struct trackType
    {
      int               leadHand,
                        leadSuit;
      int               playSuits[DDS_HANDS],
                        playRanks[DDS_HANDS];
      trickDataType     trickData;
      extCard           move[DDS_HANDS];
      int               high[DDS_HANDS];
      int               lowestWin[DDS_HANDS][DDS_SUITS];
      int               removedRanks[DDS_SUITS];
    };
    
    trackType           track[13];
    trackType           * trackp;

    movePlyType         moveList[13][DDS_HANDS];

    moveType            * mply;

    int                 lastCall[13][DDS_HANDS];

    char                funcName[13][40];

    struct moveStatType {
      int               count,
                        findex,
                        sumHits,
                        sumLengths;
    };

    struct moveStatsType {
      int               nfuncs;
      moveStatType      list[MG_NUM_FUNCTIONS];
    };

    moveStatType        trickTable[13][DDS_HANDS];

    moveStatType        trickSuitTable[13][DDS_HANDS];

    moveStatsType       trickDetailTable[13][DDS_HANDS];

    moveStatsType       trickDetailSuitTable[13][DDS_HANDS];

    moveStatsType       trickFuncTable;

    moveStatsType       trickFuncSuitTable;

    FILE                * fp;

    char                fname[80];


    void WeightAllocTrump0(
      pos               * posPoint, 
      moveType          * bestMove,
      moveType          * bestMoveTT,
      relRanksType      thrp_rel[]);

    void WeightAllocNT0(
      pos               * posPoint, 
      moveType          * bestMove,
      moveType          * bestMoveTT,
      relRanksType      thrp_rel[]);

    void WeightAllocTrumpNotvoid1(
      pos               * posPoint);

    void WeightAllocNTNotvoid1(
      pos               * posPoint);

    void WeightAllocTrumpVoid1(
      pos               * posPoint);

    void WeightAllocNTVoid1(
      pos               * posPoint);

    void WeightAllocTrumpNotvoid2(
      pos               * posPoint);

    void WeightAllocNTNotvoid2(
      pos               * posPoint);

    void WeightAllocTrumpVoid2(
      pos               * posPoint);

    void WeightAllocNTVoid2(
      pos               * posPoint);

    void WeightAllocCombinedNotvoid3(
      pos               * posPoint);

    void WeightAllocTrumpVoid3(
      pos               * posPoint);

    void WeightAllocNTVoid3(
      pos               * posPoint);

    void GetTopNumber(
      int               ris,
      int               prank,
      int               * topNumber,
      int               * mno);

    int RankForcesAce(
      int               cards4th);

    typedef void (Moves::*WeightPtr)(pos * posPoint);

    WeightPtr WeightList[16];

    inline bool WinningMove(
      moveType          * mvp1,
      extCard           * mvp2,
      int               trump);

    void PrintMove(
      movePlyType       * mply);

    void MergeSort();

    void UpdateStatsEntry(
      moveStatsType     * statp,
      int               findex,
      int               hit,
      int               len);

    char * AverageString(
      moveStatType      * statp,
      char              str[]);

    char * FullAverageString(
      moveStatType      * statp,
      char              str[]);

    void PrintTrickTable(
      moveStatType      tablep[][DDS_HANDS]);

    void PrintFunctionTable(
      moveStatsType     * tablep);

  public:
    Moves();

    ~Moves();

    void SetFile(char * fname);

    void Init(
      int               tricks,
      int               relStartHand,
      int               initialRanks[],
      int               initialSuits[],
      unsigned short int rankInSuit[DDS_HANDS][DDS_SUITS],
      int               trump,
      int               leadHand);

    void Reinit(
      int               tricks,
      int               leadHand);

    int MoveGen0(
      int               tricks,
      pos               * posPoint, 
      moveType          * bestMove,
      moveType          * bestMoveTT,
      relRanksType      thrp_rel[]);

    int MoveGen123(
      int               tricks,
      int               relHand,
      pos               * posPoint);

    int GetLength(
      int               trick,
      int               relHand);

    void MakeSpecific(
      moveType          * mply,
      int               trick,
      int               relHand);

    moveType * MakeNext(
      int               trick,
      int               relHand,
      unsigned short int winRanks[DDS_SUITS]);

    moveType * MakeNextSimple(
      int               trick,
      int               relHand);

    void Step(
      int               tricks,
      int               relHand);

    void Rewind(
      int               tricks,
      int               relHand);

    void Purge(
      int               tricks,
      int               relHand,
      moveType          forbiddenMoves[]);

    void Reward(
      int               trick,
      int               relHand);
    
    trickDataType * GetTrickData(
      int               tricks);

    void Sort(
      int               tricks,
      int               relHand);

    void PrintMoves(
      int               trick,
      int               relHand);

    void RegisterHit(
      int               tricks,
      int               relHand);
      
    void TrickToText(
      int               trick,
      char              text[]);

    void PrintTrickStats();

    void PrintTrickDetails();

    void PrintFunctionStats();

};

#endif
