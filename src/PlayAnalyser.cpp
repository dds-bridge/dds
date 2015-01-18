/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2015 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "dds.h"
#include "threadmem.h"
#include "SolverIF.h"
#include "PBN.h"
#include "Scheduler.h"

// Only single-threaded debugging here.
#define DEBUG 0

#ifdef DDS_SCHEDULER
  #define START_BLOCK_TIMER scheduler.StartBlockTimer()
  #define END_BLOCK_TIMER scheduler.EndBlockTimer()
  #define START_THREAD_TIMER(a) scheduler.StartThreadTimer(a)
  #define END_THREAD_TIMER(a) scheduler.EndThreadTimer(a)
#else
  #define START_BLOCK_TIMER 1
  #define END_BLOCK_TIMER 1
  #define START_THREAD_TIMER(a) 1
  #define END_THREAD_TIMER(a) 1
#endif

#if DEBUG
FILE * fp;
#endif



int STDCALL AnalysePlayBin(
  deal dl,
  playTraceBin play,
  solvedPlay * solvedp,
  int thrId)
{
  moveType move;
  futureTricks fut;
  int ret;

  int last_trick = (play.number + 3) / 4;
  if (last_trick > 12) last_trick = 12;
  int last_card = ((play.number + 3) % 4) + 1;
  solvedp->number = 0;

  ret = SolveBoard(dl, -1, 1, 1, &fut, thrId);
  if (ret != RETURN_NO_FAULT)
    return ret;

  solvedp->tricks[0] = 13 - fut.score[0];
  int hint = solvedp->tricks[0];
  int hintDir;

  int running_remainder = 13;
  int running_declarer = 0;
  int running_player = dl.first;
  int running_side = 1; /* defenders */
  int start_side = running_player % 2;
  int solved_declarer = running_remainder - fut.score[0];
#if DEBUG
  int initial_par = solved_declarer;
  fp = fopen("trace.txt", "a");
  fprintf(fp, "Initial solve: %d\n", initial_par);
  fprintf(fp, "no %d, Last trick %d, last card %d\n",
          play.number, last_trick, last_card);
  fprintf(fp, "%5s %5s %5s %8s %6s %6s %5s %5s %5s\n",
          "trick", "card", "rest", "declarer",
          "player", "side", "soln0", "soln1", "diff");
  fclose(fp);
#endif

  for (int trick = 1; trick <= last_trick; trick++)
  {
    int offset = 4 * (trick - 1);
    int lc = (trick == last_trick ? last_card : 4);
    int best_card = 0, best_suit = 0, best_player = 0, trump_played = 0;

    for (int card = 1; card <= lc; card++)
    {
      int suit = play.suit[offset + card - 1];
      int rr = play.rank[offset + card - 1];
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
#if DEBUG
        fp = fopen("trace.txt", "a");
        fprintf(fp, "ERR Trick %d card %d pl %d: suit %d hold %d\n",
                trick, card, running_player, suit, hold);
        fclose(fp);
#endif
        return RETURN_PLAY_FAULT;
      }

#if DEBUG
      int resp_player = running_player;
#endif
      dl.remainCards[running_player][suit] ^= hold;

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

      if ((ret = AnalyseLaterBoard(dl.first,
                                   &move, hint, hintDir, &fut, thrId))
          != RETURN_NO_FAULT)
      {
#if DEBUG
        fp = fopen("trace.txt", "a");
        fprintf(fp, "SolveBoard failed, ret %d\n", ret);
#endif
        return ret;
      }

      int new_solved_decl = running_declarer + (running_side ?
        running_remainder - fut.score[0] : fut.score[0]);

      solvedp->tricks[offset + card] = new_solved_decl;

#if DEBUG
      fp = fopen("trace.txt", "a");
      fprintf(fp, "%5d %5d %5d %8d %6c %6d %5d %5d %5d\n",
              trick, card, running_remainder, running_declarer,
              cardHand[resp_player], running_side,
              solved_declarer, new_solved_decl,
              new_solved_decl - solved_declarer);
      fclose(fp);
#endif

      solved_declarer = new_solved_decl;
    }
  }
  solvedp->number = 4 * last_trick + last_card - 3;

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

  if (ConvertPlayFromPBN(&playPBN, &play) !=
      RETURN_NO_FAULT)
    return RETURN_PLAY_FAULT;

  return AnalysePlayBin(dl, play, solvedp, thrId);
}


long pchunk = 0;
int pfail;


#if (defined(_WIN32) || defined(__CYGWIN__)) && \
    !defined(_OPENMP) && !defined(DDS_THREADS_SINGLE)

HANDLE solveAllPlayEvents[MAXNOOFTHREADS];
LONG volatile pthreadIndex;
LONG volatile pcurrent;
paramType playparam;
playparamType traceparam;

DWORD CALLBACK SolveChunkTracePlay (void *);

DWORD CALLBACK SolveChunkTracePlay (void *)
{
  solvedPlay solved[MAXNOOFBOARDS];
  int thid;

  thid = InterlockedIncrement(&pthreadIndex);

  int index;
  schedType st;
  while (1)
  {
    st = scheduler.GetNumber(thid);
    index = st.number;
    if (index == -1)
      break;

    START_THREAD_TIMER(thid);
    int res = AnalysePlayBin(
                playparam.bop->deals[index],
                traceparam.plp->plays[index],
                &solved[index],
                thid);
    END_THREAD_TIMER(thid);

    if (res == 1)
      traceparam.solvedp->solved[index] = solved[index];
    else
      pfail = res;
    /* If there are multiple errors, this will catch one of them */
  }

  if (SetEvent(solveAllPlayEvents[thid]) == 0)
    return 0;

  return 1;
}



int STDCALL AnalyseAllPlaysBin(
  boards * bop,
  playTracesBin * plp,
  solvedPlays * solvedp,
  int chunkSize)
{
  if (bop->noOfBoards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  if (bop->noOfBoards != plp->noOfBoards)
    return RETURN_UNKNOWN_FAULT;

  pchunk = chunkSize;
  pfail = 1;

  int res;
  DWORD solveAllWaitResult;

  pcurrent = 0;
  pthreadIndex = -1;

  playparam.bop = bop;
  traceparam.plp = plp;
  playparam.noOfBoards = bop->noOfBoards;
  traceparam.noOfBoards = bop->noOfBoards;
  traceparam.solvedp = solvedp;

  scheduler.RegisterTraceDepth(plp, bop->noOfBoards);
  scheduler.Register(bop, SCHEDULER_TRACE);

  for (int k = 0; k < noOfThreads; k++)
  {
    solveAllPlayEvents[k] = CreateEvent(NULL, FALSE, FALSE, 0);
    if (solveAllPlayEvents[k] == 0)
      return RETURN_THREAD_CREATE;
  }

  for (int k = 0; k < noOfThreads; k++)
  {
    res = QueueUserWorkItem(SolveChunkTracePlay, NULL,
                            WT_EXECUTELONGFUNCTION);
    if (res != 1)
      return res;
  }

  START_BLOCK_TIMER;
  solveAllWaitResult = WaitForMultipleObjects(
    static_cast<unsigned>(noOfThreads),
    solveAllPlayEvents, TRUE, INFINITE);
  END_BLOCK_TIMER;

  if (solveAllWaitResult != WAIT_OBJECT_0)
    return RETURN_THREAD_WAIT;

  for (int k = 0; k < noOfThreads; k++)
    CloseHandle(solveAllPlayEvents[k]);

  solvedp->noOfBoards = bop->noOfBoards;

  return pfail;
}

#else

int STDCALL AnalyseAllPlaysBin(
  boards * bop,
  playTracesBin * plp,
  solvedPlays * solvedp,
  int chunkSize)
{
  if (bop->noOfBoards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  if (bop->noOfBoards != plp->noOfBoards)
    return RETURN_UNKNOWN_FAULT;

  pchunk = chunkSize;
  pfail = 1;

  int res;
  solvedPlay solved[MAXNOOFBOARDS];

  scheduler.RegisterTraceDepth(plp, bop->noOfBoards);
  scheduler.Register(bop, SCHEDULER_TRACE);

#if defined (_OPENMP) && !defined(DDS_THREADS_SINGLE)
  if (omp_get_dynamic())
    omp_set_dynamic(0);
  omp_set_num_threads(noOfThreads);
#elif defined (_OPENMP)
  omp_set_num_threads(1);
#endif

  int index, thid;
  schedType st;

  START_BLOCK_TIMER;

  #pragma omp parallel default(none) shared(scheduler, bop, plp, solvedp, solved, pchunk, pfail) private(st, index, thid, res)
  {
    #pragma omp while schedule(dynamic, pchunk)

    while (1)
    {
#if defined (_OPENMP) && !defined(DDS_THREADS_SINGLE)
      thid = omp_get_thread_num();
#else
      thid = 0;
#endif

      st = scheduler.GetNumber(thid);
      index = st.number;
      if (index == -1)
        break;

      START_THREAD_TIMER(thid);
      res = AnalysePlayBin(bop->deals[index],
                           plp->plays[index],
                           &solved[index],
                           thid);
      END_THREAD_TIMER(thid);

      if (res == 1)
        solvedp->solved[index] = solved[index];
      else
        pfail = res;
    }
  }

  END_BLOCK_TIMER;

  solvedp->noOfBoards = bop->noOfBoards;

  return pfail;
}
#endif

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
    deal * dl;
    dealPBN * dlp;

    dl = &(bd.deals[k]);
    dlp = &(bopPBN->deals[k]);
    if (ConvertFromPBN(dlp->remainCards,
                       dl->remainCards) != RETURN_NO_FAULT)
      return RETURN_PBN_FAULT;

    dl->trump = dlp->trump;
    dl->first = dlp->first;

    for (int i = 0; i <= 2; i++)
    {
      dl->currentTrickSuit[i] = dlp->currentTrickSuit[i];
      dl->currentTrickRank[i] = dlp->currentTrickRank[i];
    }
  }

  pl.noOfBoards = plpPBN->noOfBoards;

  for (int k = 0; k < plpPBN->noOfBoards; k++)
  {
    if (ConvertPlayFromPBN(&plpPBN->plays[k], &pl.plays[k]) !=
        RETURN_NO_FAULT)
      return RETURN_PLAY_FAULT;
  }

  chunkSize = 1;
  return AnalyseAllPlaysBin(&bd, &pl, solvedp, chunkSize);
}

