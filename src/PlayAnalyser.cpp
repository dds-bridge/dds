/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "PlayAnalyser.h"
#include "SolverIF.h"
#include "System.h"
#include "Memory.h"
#include "Scheduler.h"
#include "PBN.h"
#include "debug.h"

using namespace std;


// Only single-threaded debugging here.
#define DEBUG 0

#if DEBUG
  ofstream fout;
#endif

struct playparamType
{
  int noOfBoards;
  playTracesBin * plp;
  solvedPlays * solvedp;
  int error;
};

paramType playparam;
playparamType traceparam;

extern System sysdep;
extern Memory memory;
extern Scheduler scheduler;


int STDCALL AnalysePlayBin(
  deal dl,
  playTraceBin play,
  solvedPlay * solvedp,
  int thrId)
{
  if (! sysdep.ThreadOK(thrId))
    return RETURN_THREAD_INDEX;

  ThreadData * thrp = memory.GetPtr(static_cast<unsigned>(thrId));

  moveType move;
  futureTricks fut;

  int ret = SolveBoardInternal(thrp, dl, -1, 1, 1, &fut);
  if (ret != RETURN_NO_FAULT)
    return ret;

  const int numTricks = ((thrp->iniDepth + 3) >> 2) + 1;
  const int numCardsPlayed = ((48 - thrp->iniDepth) % 4) + 1;

  int last_trick = (play.number + 3) / 4;
  int last_card = ((play.number + 3) % 4) + 1;
  if (last_trick >= numTricks) 
  {
    last_trick = numTricks-1;
    last_card = 4;
  }
  solvedp->number = 0;

  solvedp->tricks[0] = (numCardsPlayed % 2 == 1 ? 
    numTricks - fut.score[0] : fut.score[0]);
  int hint = solvedp->tricks[0];
  int hintDir;

  int running_remainder = numTricks;
  int running_declarer = 0;
  int running_player = dl.first;
  int running_side = 1; /* defenders */
  int start_side = running_player % 2;
  int solved_declarer = solvedp->tricks[0];
#if DEBUG
  int initial_par = solved_declarer;
  fout.open("trace.txt", ofstream::out | ofstream::app);
  fout << "Initial solve: " << initial_par << "\n";
  fout << "no " << play.number << ", Last trick " << last_trick <<
    ", last card " << last_card << "\n";
  fout << setw(5) << "trick" << setw(6) << "card" << 
    setw(6) << "rest" << setw(9) << "declarer" <<
    setw(7) << "player" << setw(7) << "side" <<
    setw(6) << "soln0" << setw(6) << "soln1" << setw(6) << "diff" << "\n";
#endif

  for (int trick = 1; trick <= last_trick; trick++)
  {
    int best_card = 0, best_suit = 0, best_player = 0, trump_played = 0;
    int lc = (trick == last_trick ? last_card : 4);

    bool haveCurrent = (numCardsPlayed > 1 && trick == 1);
    int offset = 4 * (trick - 1) - (numCardsPlayed - 1);

    for (int card = 1; card <= lc; card++)
    {
      int suit, rr;
      bool usingCurrent = (haveCurrent && card < numCardsPlayed);
      if (usingCurrent)
      {
        suit = dl.currentTrickSuit[card - 1];
        rr = dl.currentTrickRank[card - 1];
      }
      else
      {
        suit = play.suit[offset + card - 1];
        rr = play.rank[offset + card - 1];
      }
      unsigned hold = static_cast<unsigned>(bitMapRank[rr] << 2);

      move.suit = suit;
      move.rank = rr;
      move.sequence = rr;

      /* Keep track of the winner of the trick so far */
      if (card == 1)
      {
        best_card = rr;
        best_suit = suit;
        best_player = dl.first;
        trump_played = (suit == dl.trump ? 1 : 0);
      }
      else if (suit == dl.trump)
      {
        if (! trump_played || rr > best_card)
        {
          best_card = rr;
          best_suit = suit;
          best_player = running_player;
          trump_played = 1;
        }
      }
      else if (! trump_played && suit == best_suit && rr > best_card)
      {
        best_card = rr;
        best_player = running_player;
      }

      if ((dl.remainCards[running_player][suit] & hold) == 0)
      {
        if (! usingCurrent)
        {
#if DEBUG
          fout << "ERR Trick " << trick << " card " << card <<
            " pl " << running_player << ": suit " << suit <<
            " hold " << hold << "\n";
          fout.close();
#endif
          return RETURN_PLAY_FAULT;
        }
      }
      else
        dl.remainCards[running_player][suit] ^= hold;

#if DEBUG
      int resp_player = running_player;
#endif

      if (card == 4)
      {
        running_declarer += (best_player % 2 == start_side ? 0 : 1);
        running_remainder--;

        if ((dl.first + best_player) % 2 == 0)
        {
          hintDir = 0; // Same side leads again; lower bound
          hint = running_remainder - fut.score[0];
        }
        else
        {
          hintDir = 1; // Other ("our") side wins trick; upper bound
          hint = fut.score[0] - 1;
        }

        dl.first = best_player;
        running_side = (dl.first % 2 == start_side ? 1 : 0);
        running_player = dl.first;
      }
      else
      {
        running_player = (running_player + 1) % 4;
        running_side = 1 - running_side;
        hint = running_remainder - fut.score[0];
        hintDir = 0;
      }

      if (usingCurrent)
        continue;

      if ((ret = AnalyseLaterBoard(thrp, dl.first, &move, hint, 
        hintDir, &fut))
          != RETURN_NO_FAULT)
      {
#if DEBUG
        fout << "SolveBoard failed, ret " << ret << "\n";
        fout.close();
#endif
        return ret;
      }

      int new_solved_decl = running_declarer + (running_side ?
        running_remainder - fut.score[0] : fut.score[0]);

      solvedp->tricks[offset + card] = new_solved_decl;

#if DEBUG
      fout << setw(5) << trick << 
        setw(6) << card << 
        setw(6) << running_remainder << 
        setw(9) << running_declarer <<
        setw(7) << cardHand[resp_player] << 
        setw(7) << running_side <<
        setw(6) << solved_declarer << 
        setw(6) << new_solved_decl << 
        setw(6) << new_solved_decl - solved_declarer << "\n";
#endif

      solved_declarer = new_solved_decl;
    }
  }
  solvedp->number = 4 * last_trick + last_card - 3 - (numCardsPlayed - 1);

#if DEBUG
  fout.close();
#endif
  return RETURN_NO_FAULT;
}


int STDCALL AnalysePlayPBN(
  dealPBN dlPBN,
  playTracePBN playPBN,
  solvedPlay * solvedp,
  int thrId)
{
  deal dl;
  playTraceBin play;

  if (ConvertFromPBN(dlPBN.remainCards, dl.remainCards) !=
      RETURN_NO_FAULT)
    return RETURN_PBN_FAULT;

  dl.first = dlPBN.first;
  dl.trump = dlPBN.trump;
  for (int i = 0; i <= 2; i++)
  {
    dl.currentTrickSuit[i] = dlPBN.currentTrickSuit[i];
    dl.currentTrickRank[i] = dlPBN.currentTrickRank[i];
  }

  if (ConvertPlayFromPBN(playPBN, play) != RETURN_NO_FAULT)
    return RETURN_PLAY_FAULT;

  return AnalysePlayBin(dl, play, solvedp, thrId);
}


void PlaySingleCommon(
  const int thrId,
  const int bno)
{
  solvedPlay solved;

  int res = AnalysePlayBin(
    playparam.bop->deals[bno],
    traceparam.plp->plays[bno],
    &solved,
    thrId);

  // If there are multiple errors, this will catch one of them.
  if (res == 1)
    traceparam.solvedp->solved[bno] = solved;
  else
   playparam.error = res;
}


void PlayChunkCommon(const int thrId)
{
  int index;
  schedType st;

  while (1)
  {
    st = scheduler.GetNumber(thrId);
    index = st.number;
    if (index == -1)
      break;

    PlaySingleCommon(thrId, index);
  }
}


int STDCALL AnalyseAllPlaysBin(
  boards * bop,
  playTracesBin * plp,
  solvedPlays * solvedp,
  int chunkSize)
{
  UNUSED(chunkSize);

  playparam.error = 0;

  if (bop->noOfBoards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  if (bop->noOfBoards != plp->noOfBoards)
    return RETURN_UNKNOWN_FAULT;

  playparam.bop = bop;
  traceparam.plp = plp;
  playparam.noOfBoards = bop->noOfBoards;
  traceparam.noOfBoards = bop->noOfBoards;
  traceparam.solvedp = solvedp;

  scheduler.RegisterRun(DDS_RUN_TRACE, * bop, * plp);
  sysdep.RegisterRun(DDS_RUN_TRACE, * bop);

  START_BLOCK_TIMER;
  int retRun = sysdep.RunThreads();
  END_BLOCK_TIMER;

  if (retRun != RETURN_NO_FAULT)
    return retRun;

  solvedp->noOfBoards = bop->noOfBoards;

#ifdef DDS_SCHEDULER
  scheduler.PrintTiming();
#endif

  if (playparam.error == 0)
    return RETURN_NO_FAULT;
  else
    return playparam.error;
}


int STDCALL AnalyseAllPlaysPBN(
  boardsPBN * bopPBN,
  playTracesPBN * plpPBN,
  solvedPlays * solvedp,
  int chunkSize)
{
  boards bd;
  playTracesBin pl;

  bd.noOfBoards = bopPBN->noOfBoards;
  if (bd.noOfBoards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  for (int k = 0; k < bopPBN->noOfBoards; k++)
  {
    deal& dl = bd.deals[k];
    dealPBN& dlp = bopPBN->deals[k];

    if (ConvertFromPBN(dlp.remainCards,
                       dl.remainCards) != RETURN_NO_FAULT)
      return RETURN_PBN_FAULT;

    dl.trump = dlp.trump;
    dl.first = dlp.first;

    for (int i = 0; i <= 2; i++)
    {
      dl.currentTrickSuit[i] = dlp.currentTrickSuit[i];
      dl.currentTrickRank[i] = dlp.currentTrickRank[i];
    }
  }

  pl.noOfBoards = plpPBN->noOfBoards;

  for (int k = 0; k < plpPBN->noOfBoards; k++)
  {
    if (ConvertPlayFromPBN(plpPBN->plays[k], pl.plays[k]) !=
        RETURN_NO_FAULT)
      return RETURN_PLAY_FAULT;
  }

  chunkSize = 1;
  return AnalyseAllPlaysBin(&bd, &pl, solvedp, chunkSize);
}


void DetectPlayDuplicates(
  const boards& bds,
  vector<int>& uniques,
  vector<int>& crossrefs)
{
  // This dummy function is there for consistency in System.cpp.
  // In practice there is not much point in deteting play repeats,
  // as it is highly unlikely that the play went identically at
  // two tables.

  uniques.resize(static_cast<unsigned>(bds.noOfBoards));
  crossrefs.resize(static_cast<unsigned>(bds.noOfBoards));
  for (unsigned i = 0; i < uniques.size(); i++)
  {
    uniques[i] = static_cast<int>(i);
    crossrefs[i] = -1;
  }
}


void CopyPlaySingle(const vector<int>& crossrefs)
{
  UNUSED(crossrefs);
}

