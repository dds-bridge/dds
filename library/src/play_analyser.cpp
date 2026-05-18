/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "play_analyser.hpp"
#include <solver_if.hpp>
#include <pbn.hpp>
#include <solver_context/solver_context.hpp>
#include <system/memory.hpp>
#include <system/scheduler.hpp>
#include <system/system.hpp>

using namespace std;


// Only single-threaded debugging here.
#define DEBUG 0

#if DEBUG
  #include <utility/debug.h>
  ofstream fout;
#endif

struct playparamType
{
  int no_of_boards;
  PlayTracesBin const * plp;
  SolvedPlays * solvedp;
  int error;
};

ParamType playparam;
playparamType traceparam;

extern System sysdep;
extern Memory memory;
extern Scheduler scheduler;


/**
 * @brief Analyze a sequence of played cards (binary format) and determine the tricks taken.
 *
 * This function simulates play of a bridge Deal according to the provided play trace,
 * using double dummy analysis to determine the number of tricks won at each step.
 *
 * @param dl The Deal to analyze
 * @param play The sequence of played cards (binary format)
 * @param solvedp Pointer to result structure for solved play
 * @param thrId Index of thread to use
 * @return 1 on success, error code otherwise
 */
int STDCALL AnalysePlayBin(
  Deal dl,
  PlayTraceBin play,
  SolvedPlay * solvedp,
  int thrId)
{
  if (! sysdep.thread_ok(thrId))
    return RETURN_THREAD_INDEX;

  // Create an owned context for this analysis and obtain its ThreadData.
  SolverContext outer_ctx;
  auto thrp = outer_ctx.thread();

  MoveType move;
  FutureTricks fut;

  int ret = solve_board_internal(outer_ctx, dl, -1, 1, 1, &fut);
  if (ret != RETURN_NO_FAULT)
    return ret;
  SolverContext& ctx = outer_ctx;
  const int ini_depth = ctx.search().ini_depth();
  const int numTricks = ((ini_depth + 3) >> 2) + 1;
  const int numCardsPlayed = ((48 - ini_depth) % 4) + 1;

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
#if DEBUG
  int solved_declarer = solvedp->tricks[0];
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
      unsigned hold = static_cast<unsigned>(bit_map_rank[rr] << 2);

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

      if ((ret = analyse_later_board(thrp, dl.first, &move, hint, 
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
        setw(7) << card_hand[resp_player] << 
        setw(7) << running_side <<
        setw(6) << solved_declarer << 
        setw(6) << new_solved_decl << 
        setw(6) << new_solved_decl - solved_declarer << "\n";
        solved_declarer = new_solved_decl;
#endif
      
    }
  }
  solvedp->number = 4 * last_trick + last_card - 3 - (numCardsPlayed - 1);

#if DEBUG
  fout.close();
#endif
  return RETURN_NO_FAULT;
}


/**
 * @brief Analyze a sequence of played cards (PBN format) and determine the tricks taken.
 *
 * This function converts a PBN-format Deal and play trace to internal format,
 * then simulates play using double dummy analysis to determine the number of tricks won.
 *
 * @param dlPBN The Deal to analyze (PBN format)
 * @param playPBN The sequence of played cards (PBN format)
 * @param solvedp Pointer to result structure for solved play
 * @param thrId Index of thread to use
 * @return 1 on success, error code otherwise
 */
int STDCALL AnalysePlayPBN(
  DealPBN dlPBN,
  PlayTracePBN playPBN,
  SolvedPlay * solvedp,
  int thrId)
{
  Deal dl;
  PlayTraceBin play;

  if (convert_from_pbn(dlPBN.remainCards, dl.remainCards) !=
      RETURN_NO_FAULT)
    return RETURN_PBN_FAULT;

  dl.first = dlPBN.first;
  dl.trump = dlPBN.trump;
  for (int i = 0; i <= 2; i++)
  {
    dl.currentTrickSuit[i] = dlPBN.currentTrickSuit[i];
    dl.currentTrickRank[i] = dlPBN.currentTrickRank[i];
  }

  if (convert_play_from_pbn(playPBN, play) != RETURN_NO_FAULT)
    return RETURN_PLAY_FAULT;

  return AnalysePlayBin(dl, play, solvedp, thrId);
}


void play_single_common(
  const int thrId,
  const int bno)
{
  SolvedPlay solved;

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


void play_chunk_common(const int thrId)
{
  int index;
  schedType st;

  while (1)
  {
    st = scheduler.GetNumber(thrId);
    index = st.number;
    if (index == -1)
      break;

    play_single_common(thrId, index);
  }
}


int STDCALL AnalyseAllPlaysBin(
  Boards const * bop,
  PlayTracesBin const * plp,
  SolvedPlays * solvedp,
  [[maybe_unused]] int chunkSize)
{
  playparam.error = 0;

  if (bop->no_of_boards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  if (bop->no_of_boards != plp->no_of_boards)
    return RETURN_UNKNOWN_FAULT;

  playparam.bop = bop;
  traceparam.plp = plp;
  playparam.no_of_boards = bop->no_of_boards;
  traceparam.no_of_boards = bop->no_of_boards;
  traceparam.solvedp = solvedp;

  scheduler.RegisterRun(RunMode::DDS_RUN_TRACE, * bop, * plp);

  START_BLOCK_TIMER;
  
  // Sequential execution: analyze each play in order
  // Thread ID 0 is used for all plays (single-threaded)
  for (int bno = 0; bno < bop->no_of_boards; bno++) {
    play_single_common(0, bno);
    if (playparam.error != 0)
      return playparam.error;
  }
  
  END_BLOCK_TIMER;

  solvedp->no_of_boards = bop->no_of_boards;

#ifdef DDS_SCHEDULER
  scheduler.PrintTiming();
#endif

  return RETURN_NO_FAULT;
}


int STDCALL AnalyseAllPlaysPBN(
  BoardsPBN const * bopPBN,
  PlayTracesPBN const * plpPBN,
  SolvedPlays * solvedp,
  int chunkSize)
{
  Boards bd;
  PlayTracesBin pl;

  bd.no_of_boards = bopPBN->no_of_boards;
  if (bd.no_of_boards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  for (int k = 0; k < bopPBN->no_of_boards; k++)
  {
    Deal& dl = bd.deals[k];
    DealPBN const & dlp = bopPBN->deals[k];

    if (convert_from_pbn(dlp.remainCards,
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

  pl.no_of_boards = plpPBN->no_of_boards;

  for (int k = 0; k < plpPBN->no_of_boards; k++)
  {
    if (convert_play_from_pbn(plpPBN->plays[k], pl.plays[k]) !=
        RETURN_NO_FAULT)
      return RETURN_PLAY_FAULT;
  }

  chunkSize = 1;
  return AnalyseAllPlaysBin(&bd, &pl, solvedp, chunkSize);
}


void detect_play_duplicates(
  const Boards& bds,
  vector<int>& uniques,
  vector<int>& crossrefs)
{
  // This dummy function is there for consistency in System.cpp.
  // In practice there is not much point in deteting play repeats,
  // as it is highly unlikely that the play went identically at
  // two tables.

  uniques.resize(static_cast<unsigned>(bds.no_of_boards));
  crossrefs.resize(static_cast<unsigned>(bds.no_of_boards));
  for (unsigned i = 0; i < uniques.size(); i++)
  {
    uniques[i] = static_cast<int>(i);
    crossrefs[i] = -1;
  }
}


void copy_play_single([[maybe_unused]] const vector<int>& crossrefs)
{ }

