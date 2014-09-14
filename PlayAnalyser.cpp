/*
   DDS 2.6.0   A bridge double dummy solver.
   Copyright (C) 2006-2014 by Bo Haglund
   Cleanups and porting to Linux and MacOSX (C) 2006 by Alex Martelli.
   The code for calculation of par score / contracts is based upon the
   perl code written by Matthew Kidd for ACBLmerge. He has kindly given 
   me permission to include a C++ adaptation in DDS. 
*/

/*
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or 
   implied.  See the License for the specific language governing 
   permissions and limitations under the License.
*/


/* The PlayAnalyser was written by Sören Hein. Many thanks for
   allowing me to include it in DDS. */


/*#include "stdafx.h"*/

#include "PlayAnalyser.h"
#include "dll.h"
#include "dds.h"

#define DEBUG 0

FILE *fp;


int STDCALL AnalysePlayBin(
  struct deal		dl,
  struct playTraceBin	play,
  struct solvedPlay	* solvedp,
  int			thrId)
{
  struct futureTricks fut;
  int ret;

  int last_trick  = (play.number+3) / 4;
  if (last_trick > 12) last_trick = 12;
  int last_card   = ((play.number+3) % 4) + 1;
  solvedp->number = 0;

  ret = SolveBoard(dl, -1, 3, 1, &fut, thrId);
  if (ret != RETURN_NO_FAULT)
    return ret;

  solvedp->tricks[0] = 13 - fut.score[0];

  int running_remainder = 13;
  int running_declarer  =  0;
  int running_player    = dl.first;
  int running_side      = 1; /* defenders */
  int start_side        = running_player % 2;
  int solved_declarer   = running_remainder - fut.score[0];
  int declarer          = (dl.first + 3) % 4;
  int dummy             = (dl.first + 1) % 4;
  int initial_par       = solved_declarer;
  if (DEBUG) 
  {
    fp = fopen("trace.txt", "a");
    fprintf(fp, "Initial solve: %d\n", initial_par);
    fprintf(fp, "no %d, Last trick %d, last card %d\n", 
      play.number, last_trick, last_card);
    fprintf(fp, "%5s %5s %5s %8s %6s %6s %5s %5s %5s\n",
      "trick", "card", "rest", "declarer",
      "player", "side", "soln0", "soln1", "diff");
    fclose(fp);
  }

  for (int trick = 1; trick <= last_trick; trick++)
  {
    int offset = 4 * (trick-1);
    int lc = (trick == last_trick ? last_card : 4);
    for (int card = 1; card <= lc; card++)
    {
      int suit = play.suit[offset + card - 1];
      int rr   = play.rank[offset + card - 1];
      int hold = bitMapRank[rr] << 2;

      /* Keep track of the winner of the trick so far */
      int best_card, best_suit, best_player, trump_played;
      if (card == 1)
      {
        best_card    = rr;
        best_suit    = suit;
        best_player  = dl.first;
        trump_played = (suit == dl.trump ? 1 : 0);
      }
      else if (suit == dl.trump)
      {
        if (! trump_played || rr > best_card)
	{
          best_card    = rr;
	  best_suit    = suit;
	  best_player  = running_player;
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
	if (DEBUG)
	{
          fp = fopen("trace.txt", "a");
          fprintf(fp, "ERR Trick %d card %d pl %d: suit %d hold %d\n",
	    trick, card, running_player, suit, hold);
	  fclose(fp);
	}
        return RETURN_PLAY_FAULT;
      }

      int resp_player = running_player;
      dl.remainCards[running_player][suit] ^= hold;

      if (card == 4)
      {
        running_declarer +=  (best_player % 2 == start_side ? 0 : 1);
	running_remainder--;

        dl.first = best_player;
	running_side = (dl.first % 2 == start_side ? 1 : 0);
	running_player = dl.first;
        for (int i = 0; i <= 2; i++)
        {
          dl.currentTrickSuit[i] = 0;
          dl.currentTrickRank[i] = 0;
        }
      }
      else
      {
        running_player = (running_player + 1) % 4;
	running_side   = 1 - running_side;
        dl.currentTrickSuit[card-1] = suit;
        dl.currentTrickRank[card-1] = rr;
      }

      if ((ret = SolveBoard(dl, -1, 2/*3*/, 1, &fut, thrId)) 
        != RETURN_NO_FAULT)
      {
        if (DEBUG)
	{
	  fp = fopen("trace.txt", "a");
	  fprintf(fp, "SolveBoard failed, ret %d\n", ret);
	}
        return ret;
      }

      int new_solved_decl = running_declarer + (running_side ?
        running_remainder - fut.score[0] : fut.score[0]);

      solvedp->tricks[offset + card] = new_solved_decl;

      if (DEBUG)
      {
        fp = fopen("trace.txt", "a");
        fprintf(fp, "%5d %5d %5d %8d %6c %6d %5d %5d %5d\n",
          trick, card, running_remainder, running_declarer,
	  cardHand[resp_player], running_side,
	  solved_declarer, new_solved_decl, 
	  new_solved_decl - solved_declarer);
	fclose(fp);
      }

      solved_declarer = new_solved_decl;
    }
  }
  solvedp->number = 4 * last_trick + last_card - 3;

  return RETURN_NO_FAULT;
}


int STDCALL AnalysePlayPBN(
  struct dealPBN	dlPBN,
  struct playTracePBN	playPBN,
  struct solvedPlay	* solvedp,
  int			thrId)
{
  int ConvertPlayFromPBN(
  struct playTracePBN	*playPBN,
  struct playTraceBin	*playBin);

  struct deal		dl;
  struct playTraceBin	play;

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


int ConvertPlayFromPBN(
  struct playTracePBN	*playPBN,
  struct playTraceBin	*playBin)
{
  int n = playPBN->number;

  if (n < 0 || n > 52)
    return RETURN_PLAY_FAULT;

  playBin->number = n;

  for (int i = 0; i < 2*n; i += 2)
  {
    char suit = playPBN->cards[i];
    int s;

    if (suit == 's' || suit == 'S')
      s = 0;
    else if (suit == 'h' || suit == 'H')
      s = 1;
    else if (suit == 'd' || suit == 'D')
      s = 2;
    else if (suit == 'c' || suit == 'C')
      s = 3;
    else
      return RETURN_PLAY_FAULT;
    playBin->suit[i >> 1] = s;

    int rank = IsCard(playPBN->cards[i+1]);
    if (rank == 0)
      return RETURN_PLAY_FAULT;

    // playBin->rank[i >> 1] = bitMapRank[rank];
    playBin->rank[i >> 1] = rank;
  }
  return RETURN_NO_FAULT;
}


long 			pchunk = 0;
int			pfail;


#if defined(_WIN32) && !defined(_OPENMP) && !defined(DDS_THREADS_SINGLE)

HANDLE 			solveAllPlayEvents[MAXNOOFTHREADS];
LONG volatile 		pthreadIndex;
LONG volatile 		pcurrent;
struct playparamType 	playparam;

DWORD CALLBACK SolveChunkTracePlay (void *)
{
  struct solvedPlay     solved[MAXNOOFBOARDS];
  int thid;
  long j;

  thid = InterlockedIncrement(&pthreadIndex);

  while ((j = InterlockedExchangeAdd(&pcurrent, pchunk)) < 
          playparam.noOfBoards)
  {

    for (int k = 0; k < pchunk && j + k < playparam.noOfBoards; k++)
    {
      int res = AnalysePlayBin(playparam.bop->deals[j + k], 
                             playparam.plp->plays[j + k],
                             &solved[j + k],
			     thid);

      if (res == 1)
        playparam.solvedp->solved[j + k] = solved[j + k];
      else
        pfail = res;
        /* If there are multiple errors, this will catch one of them */
    }
  }

  if (SetEvent(solveAllPlayEvents[thid]) == 0)
    return 0;

  return 1;
}


int STDCALL AnalyseAllPlaysBin(
  struct boards		* bop,
  struct playTracesBin	* plp,
  struct solvedPlays	* solvedp,
  int			chunkSize)
{
  if (bop->noOfBoards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  if (bop->noOfBoards != plp->noOfBoards)
    return RETURN_UNKNOWN_FAULT;

  pchunk = chunkSize;
  pfail  = 1;

  DWORD res;
  DWORD solveAllWaitResult;

  pcurrent = 0;
  pthreadIndex = -1;

  playparam.bop = bop;
  playparam.plp = plp;
  playparam.noOfBoards = bop->noOfBoards;
  playparam.solvedp = solvedp;

  for (int k = 0; k < noOfCores; k++)
  {
    solveAllPlayEvents[k] = CreateEvent(NULL, FALSE, FALSE, 0);
    if (solveAllPlayEvents[k] == 0)
      return RETURN_THREAD_CREATE;
  }

  for (int k = 0; k < noOfCores; k++)
  {
    res = QueueUserWorkItem(SolveChunkTracePlay, NULL, 
                            WT_EXECUTELONGFUNCTION);
    if (res != 1)
      return res;
  }

  solveAllWaitResult = WaitForMultipleObjects(noOfCores, 
    solveAllPlayEvents, TRUE, INFINITE);

  if (solveAllWaitResult != WAIT_OBJECT_0)
    return RETURN_THREAD_WAIT;

  for (int k = 0; k < noOfCores; k++)
    CloseHandle(solveAllPlayEvents[k]);

  solvedp->noOfBoards = bop->noOfBoards;

  return pfail;
}

#else

int STDCALL AnalyseAllPlaysBin(
  struct boards		* bop,
  struct playTracesBin	* plp,
  struct solvedPlays	* solvedp,
  int			chunkSize)
{
  if (bop->noOfBoards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  if (bop->noOfBoards != plp->noOfBoards)
    return RETURN_UNKNOWN_FAULT;

  pchunk = chunkSize;
  pfail  = 1;

  int res;
  struct solvedPlay     solved[MAXNOOFBOARDS];

#if defined (_OPENMP) && !defined(DDS_THREADS_SINGLE)
  if (omp_get_dynamic())
    omp_set_dynamic(0);
  omp_set_num_threads(noOfCores);	
#elif defined (_OPENMP)
  omp_set_num_threads(1);
#endif

  #pragma omp parallel shared(bop, plp, solvedp, pchunk, pfail)
  {
    #pragma omp for schedule(dynamic, pchunk)

    for (int k = 0; k < bop->noOfBoards; k++)
    {
      res = AnalysePlayBin(bop->deals[k], 
                         plp->plays[k],
                         &solved[k],

#if defined (_OPENMP) && !defined(DDS_THREADS_SINGLE) 

                         omp_get_thread_num()
#else
			 0
#endif
			 );

      if (res == 1)
        solvedp->solved[k] = solved[k];
      else
        pfail = res;
    }
  }

  solvedp->noOfBoards = bop->noOfBoards;

  return pfail;
}
#endif

int STDCALL AnalyseAllPlaysPBN(
  struct boardsPBN	* bopPBN,
  struct playTracesPBN	* plpPBN,
  struct solvedPlays	* solvedp,
  int			chunkSize)
{
  struct boards		bd;
  struct playTracesBin	pl;

  bd.noOfBoards = bopPBN->noOfBoards;

  for (int k = 0; k < bopPBN->noOfBoards; k++)
  {
    struct deal    * dl;
    struct dealPBN * dlp;

    dl  = &(bd.deals[k]);
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

