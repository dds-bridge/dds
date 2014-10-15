/* 
   DDS 2.7.0   A bridge double dummy solver.
   Copyright (C) 2006-2014 by Bo Haglund   
   Cleanups and porting to Linux and MacOSX (C) 2006 by Alex Martelli.
   The code for calculation of par score / contracts is based upon the
   perl code written by Matthew Kidd for ACBLmerge. He has kindly given
   permission to include a C++ adaptation in DDS.
   						
   The PlayAnalyser analyses the played cards of the deal and presents 
   their double dummy values. The par calculation function DealerPar 
   provides an alternative way of calculating and presenting par 
   results.  Both these functions have been written by Soren Hein.
   He has also made numerous contributions to the code, especially in 
   the initialization part.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
   http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or 
   implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


#include "dds.h"
#include "Init.h"
#include "Stats.h"
#include "ABsearch.h"

inline bool WinningMove(
  struct moveType * mvp1,
  struct moveType * mvp2,
  int trump);

void InitConstants();
void InitDebugFiles();

struct localVarType	localVar[MAXNOOFTHREADS];
int noOfThreads;

int lho[DDS_HANDS]     = { 1, 2, 3, 0 };
int rho[DDS_HANDS]     = { 3, 0, 1, 2 };
int partner[DDS_HANDS] = { 2, 3, 0, 1 };

// bitMapRank[absolute rank] is the absolute suit corresponding
// to that rank.  The absolute rank is 2 .. 14, but it is useful
// for some reason that I have forgotten to have number 15
// set as well :-).

unsigned short int bitMapRank[16] =
{
  0x0000, 0x0000, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020,
  0x0040, 0x0080, 0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000
};

unsigned char cardRank[16] =
{ 
  'x', 'x', '2', '3', '4', '5', '6', '7',
  '8', '9', 'T', 'J', 'Q', 'K', 'A', '-'
};

unsigned char cardSuit[DDS_STRAINS] =
{ 
  'S', 'H', 'D', 'C', 'N' 
};


unsigned char cardHand[DDS_HANDS] = 
{ 
  'N', 'E', 'S', 'W'
};

// There is no particular reason for the different types here,
// other than historical ones.  They could all be char's for
// memory reasons, or all be int's for performance reasons.

int			highestRank[8192];
int			counttable[8192];
char			relRank[8192][15];
unsigned short int	winRanks[8192][14];


int _initialized = 0;

void STDCALL SetMaxThreads(
  int 			userThreads)
{
  if (! _initialized)
  {
    noOfThreads = 0;

    InitConstants();

    InitDebugFiles();
  }
  _initialized = 1;

  InitTimer();
  InitTimerList();

  // First figure out how much memory we have available
  // and how many cores the system has.
  int oldNoOfThreads = noOfThreads;
  unsigned long long kilobytesFree = 0;
  int ncores = 1;

#ifdef DDS_THREADS_SINGLE
  noOfThreads = 1;
#else
  #if defined(_WIN32) || defined(__CYGWIN__)
    /* Using GlobalMemoryStatusEx instead of GlobalMemoryStatus
       was suggested by Lorne Anderson. */
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);	
    kilobytesFree = (unsigned long long) statex.ullTotalPhys / 1024;

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    ncores = (int) sysinfo.dwNumberOfProcessors;
  #endif

  #ifdef __linux__   
    /* The code for linux was suggested by Antony Lee. */
    FILE* fifo = popen(
      "free -k | tail -n+3 | head -n1 | awk '{print $NF}'", "r");
    fscanf(fifo, "%lld", &kilobytesFree);
    fclose(fifo);

    ncores = sysconf(_SC_NPROCESSORS_ONLN);
  #endif
#endif

  // 70%, capped at 2 GB.
  int kilobytesUsable = (int) (0.70 * kilobytesFree);
  if (kilobytesUsable > 2000000)
    kilobytesUsable = 2000000;

  if (userThreads)
    noOfThreads = Min(ncores, userThreads);
  else
    noOfThreads = ncores;

  int deltaThreads = noOfThreads - oldNoOfThreads;

  int mem_def, mem_max;

  if (deltaThreads <= 0)
  {
    // We already have the memory.
    mem_def = THREADMEM_DEF_MB;
    mem_max = THREADMEM_MAX_MB;
  }
  else if (kilobytesUsable == 0 || noOfThreads == 1)
  {
    // Take our chances with default values.
    mem_def = THREADMEM_DEF_MB;
    mem_max = THREADMEM_MAX_MB;
  }
  else if (kilobytesUsable >= 1024 * THREADMEM_MAX_MB * deltaThreads)
  {
    // Comfortable.
    mem_def = THREADMEM_DEF_MB;
    mem_max = THREADMEM_MAX_MB;
  }
  else if (kilobytesUsable >= 1024 * THREADMEM_DEF_MB * deltaThreads)
  {
    // Slightly less comfortable, cap the maximum,
    // but make the maximum number of threads.
    mem_def = THREADMEM_DEF_MB;
    mem_max = THREADMEM_DEF_MB;
  }
  else
  {
    // Limit the number of threads to the available memory.
    int fittingThreads = (int) kilobytesUsable /
      ( (double) 1024. * THREADMEM_DEF_MB );
    
    noOfThreads = Max(fittingThreads, 1);
    mem_def = THREADMEM_DEF_MB;
    mem_max = THREADMEM_DEF_MB;
  }

  for (int k = 0; k < noOfThreads; k++)
  {
    localVar[k].transTable.SetMemoryDefault(mem_def);
    localVar[k].transTable.SetMemoryMaximum(mem_max);
  }

  if (noOfThreads == oldNoOfThreads)
  {
    // Must already have TT allocated.
  }
  else if (noOfThreads > oldNoOfThreads)
  {
    for (int k = oldNoOfThreads; k < noOfThreads; k++)
      localVar[k].transTable.MakeTT();
  }
  else
  {
    for (int k = noOfThreads; k < oldNoOfThreads; k++)
      localVar[k].transTable.ReturnAllMemory();
  }

}


void InitConstants()
{
  // highestRank[aggr] is the highest absolute rank in the
  // suit represented by aggr.  The absolute rank is 2 .. 14.
  highestRank[0] = 0;
  for (int aggr = 1; aggr < 8192; aggr++)
  {
    for (int r = 14; r >= 2; r--)
    {
      if (aggr & bitMapRank[r])
      {
        highestRank[aggr] = r;
        break;
      }
    }
  }

  /* The use of the counttable to give the number of bits set to
  one in an integer follows an implementation by Thomas Andrews. */

  // counttable[aggr] is the number of '1' bits (binary weight)
  // in aggr.
  for (int aggr = 0; aggr < 8192; aggr++)
  {
    counttable[aggr] = 0;
    for (int r = 0; r < 13; r++)
    {
      if (aggr & (1 << r))
      {
        counttable[aggr]++;
      }
    }
  }

  // relRank[aggr][absolute rank] is the relative rank of
  // that absolute rank in the suit represented by aggr.
  // The relative rank is 2 .. 14.
  memset(relRank[0], 0, 15);
  for (int aggr = 1; aggr < 8192; aggr++)
  {
    int ord = 0;
    for (int r = 14; r >= 2; r--)
    {
      if (aggr & bitMapRank[r])
      {
        ord++;
	relRank[aggr][r] = ord;
      }
    }
  }

  // winRanks[aggr][leastWin] is the absolute suit represented
  // by aggr, but limited to its top "leastWin" bits.
  for (int aggr = 0; aggr < 8192; aggr++)
  {
    winRanks[aggr][0] = 0;
    for (int leastWin = 1; leastWin < 14; leastWin++)
    {
      int res       = 0;
      int nextBitNo = 1;
      for (int r = 14; r >= 2; r--)
      {
        if (aggr & bitMapRank[r])
        {
          if (nextBitNo <= leastWin)
          {
            res |= bitMapRank[r];
            nextBitNo++;
          }
          else
            break;
        }
      }
      winRanks[aggr][leastWin] = res;
    }
  }
}


void InitDebugFiles()
{
  for (int k = 0; k < noOfThreads; k++)
  {
#ifdef DDS_TOP_LEVEL
    InitFileTopLevel(k);
#endif

#ifdef DDS_AB_STATS
  InitFileABstats(k);
#endif

#ifdef DDS_AB_HITS
    InitFilesABhits(k);
#endif

#ifdef DDS_TT_STATS
  InitFileTTstats(k);
#endif

#ifdef DDS_TIMING
    InitFileTimer(k);
#endif
  }
}


void CloseDebugFiles()
{
  for (int k = 0; k < noOfThreads; k++)
  {
#ifdef DDS_TOP_LEVEL
    CloseFileTopLevel(k);
#endif

#ifdef DDS_AB_HITS
    CloseFilesABhits(k);
#endif
  }
}


void InitGame(
  int 			gameNo, 
  bool 			moveTreeFlag, 
  int 			first, 
  int 			handRelFirst, 
  int 			thrId)
{
  unsigned int 		topBitRank = 1;
  unsigned int 		topBitNo   = 2;
  struct localVarType 	* thrp = &localVar[thrId];

  if (thrp->newDeal)
  {
    /* Initialization of the rel structure is implemented
       according to a solution given by Thomas Andrews */

    for (int s = 0; s < DDS_SUITS; s++)
    {
      thrp->iniPosition.aggr[s] = 0;
      for (int h = 0; h < DDS_HANDS; h++)
      {
        thrp->iniPosition.rankInSuit[h][s] = thrp->game.suit[h][s];
        thrp->iniPosition.aggr[s]         |= thrp->game.suit[h][s];
      }
    }

    // rel[aggr].absRank[absolute rank][suit].hand is the hand
    // (N = 0, E = 1 etc.) which holds the absolute rank in
    // the suit characterized by aggr.
    // rel[aggr].absRank[absolute rank][suit].rank is the
    // relative rank of that card.

    for (int s = 0; s < DDS_SUITS; s++)
    {
      for (int ord = 1; ord <= 13; ord++)
      {
        thrp->rel[0].absRank[ord][s].hand = -1;
        thrp->rel[0].absRank[ord][s].rank = 0;
      }
    }

    // handLookup[suit][absolute rank] is the hand (N = 0 etc.)
    // holding the absolute rank in suit.

    int handLookup[DDS_SUITS][15];
    for (int s = 0; s < DDS_SUITS; s++)
    {
      for (int r = 14; r >= 2; r--)
      {
        handLookup[s][r] = -1;
	for (int h = 0; h < DDS_HANDS; h++)
	{
	  if (thrp->game.suit[h][s] & bitMapRank[r])
	  {
	    handLookup[s][r] = h;
	    break;
	  }
	}
      }
    }

    thrp->transTable.Init(handLookup);

    struct relRanksType * relp;
    for (int aggr = 1; aggr < 8192; aggr++)
    {
      if (aggr >= (topBitRank << 1))
      {
        /* Next top bit */
        topBitRank <<= 1;
	topBitNo++;
      }

      thrp->rel[aggr] = thrp->rel[aggr ^ topBitRank];
      relp = &thrp->rel[aggr];

      int weight = counttable[aggr];
      for (int c = weight; c >= 2; c--)
      {
        for (int s = 0; s < DDS_SUITS; s++)
	{
          relp->absRank[c][s].hand = relp->absRank[c-1][s].hand;
          relp->absRank[c][s].rank = relp->absRank[c-1][s].rank;
	}
      }
      for (int s = 0; s < DDS_SUITS; s++)
      {
        relp->absRank[1][s].hand = handLookup[s][topBitNo];
        relp->absRank[1][s].rank = topBitNo;
      }
    }
  }

  thrp->iniPosition.first[thrp->game.noOfCards - 4] = first;
  thrp->iniPosition.handRelFirst = handRelFirst;
  thrp->lookAheadPos = thrp->iniPosition;

  thrp->estTricks[1] = 6;
  thrp->estTricks[3] = 6;
  thrp->estTricks[0] = 7;
  thrp->estTricks[2] = 7;

  InitSearch(&thrp->lookAheadPos, thrp->game.noOfCards - 4,
    thrp->initialMoves, first, moveTreeFlag, thrId);

}


void InitSearch(
  struct pos 		* posPoint, 
  int 			depth, 
  struct moveType 	startMoves[],
  int 			first, 
  bool 			mtd, 
  int 			thrId)
{

  int handRelFirst, maxAgg, maxHand = 0;
  int noOfStartMoves; /* Number of start moves in the 1st trick */
  int hand[3], suit[3], rank[3];
  struct moveType move;
  unsigned short int startMovesBitMap[DDS_HANDS][DDS_SUITS]; 
  unsigned short int aggHand[DDS_HANDS][DDS_SUITS];

  struct localVarType 	* thrp = &localVar[thrId];

  for (int h = 0; h < DDS_HANDS; h++)
    for (int s = 0; s < DDS_SUITS; s++)
      startMovesBitMap[h][s] = 0;

  handRelFirst = posPoint->handRelFirst;
  noOfStartMoves = handRelFirst;

  for (int k = 0; k <= 2; k++)
  {
    hand[k] = handId(first, k);
    suit[k] = startMoves[k].suit;
    rank[k] = startMoves[k].rank;
    if (k < noOfStartMoves)
      startMovesBitMap[hand[k]][suit[k]] |= bitMapRank[rank[k]];
  }

  for (int d = 0; d <= 49; d++)
  {
    thrp->bestMove  [d].rank = 0;
    thrp->bestMoveTT[d].rank = 0;
  }

  if (((handId(first, handRelFirst)) == 0) ||
      ((handId(first, handRelFirst)) == 2))
  {
    thrp->nodeTypeStore[0] = MAXNODE;
    thrp->nodeTypeStore[1] = MINNODE;
    thrp->nodeTypeStore[2] = MAXNODE;
    thrp->nodeTypeStore[3] = MINNODE;
  }
  else
  {
    thrp->nodeTypeStore[0] = MINNODE;
    thrp->nodeTypeStore[1] = MAXNODE;
    thrp->nodeTypeStore[2] = MINNODE;
    thrp->nodeTypeStore[3] = MAXNODE;
  }

  int k = noOfStartMoves;
  posPoint->first[depth] = first;
  posPoint->handRelFirst = k;

  assert((posPoint->handRelFirst >= 0) && 
         (posPoint->handRelFirst <= 3));

  posPoint->tricksMAX = 0;

  if (k > 0)
  {
    posPoint->move[depth + k] = startMoves[k - 1];
    move = startMoves[k - 1];
  }

  posPoint->high[depth + k] = first;

  while (k > 0)
  {
    thrp->movePly[depth + k].current = 0;
    thrp->movePly[depth + k].last = 0;
    thrp->movePly[depth + k].move[0].suit = startMoves[k - 1].suit;
    thrp->movePly[depth + k].move[0].rank = startMoves[k - 1].rank;
    if (k < noOfStartMoves)  /* If there is more than one start move */
    {
      if (WinningMove(&startMoves[k - 1], &move, thrp->trump))
      {
        posPoint->move[depth + k].suit = startMoves[k - 1].suit;
        posPoint->move[depth + k].rank = startMoves[k - 1].rank;
        posPoint->high[depth + k] = handId(first, noOfStartMoves - k);
        move = posPoint->move[depth + k];
      }
      else
      {
        posPoint->move[depth + k] = posPoint->move[depth + k + 1];
        posPoint->high[depth + k] = posPoint->high[depth + k + 1];
      }
    }
    k--;
  }

  for (int s = 0; s < DDS_SUITS; s++)
    posPoint->removedRanks[s] = 0;

  for (int s = 0; s < DDS_SUITS; s++)
    for (int h = 0; h < DDS_HANDS; h++)
      posPoint->removedRanks[s] |= posPoint->rankInSuit[h][s];

  for (int s = 0; s < DDS_SUITS; s++)
    posPoint->removedRanks[s] = ~(posPoint->removedRanks[s]);

  for (int s = 0; s < DDS_SUITS; s++)
    for (int h = 0; h < DDS_HANDS; h++)
      posPoint->removedRanks[s] &= (~startMovesBitMap[h][s]);

  for (int s = 0; s < DDS_SUITS; s++)
    thrp->iniRemovedRanks[s] = posPoint->removedRanks[s];

  /* Initialize winning and second best ranks */
  for (int s = 0; s < DDS_SUITS; s++)
  {
    maxAgg = 0;
    for (int h = 0; h < DDS_HANDS; h++)
    {
      aggHand[h][s] = startMovesBitMap[h][s] | thrp->game.suit[h][s];
      if (aggHand[h][s] > maxAgg)
      {
        maxAgg = aggHand[h][s];
        maxHand = h;
      }
    }

    if (maxAgg != 0)
    {
      posPoint->winner[s].hand = maxHand;
      k = highestRank[aggHand[maxHand][s]];
      posPoint->winner[s].rank = k;

      maxAgg = 0;
      for (int h = 0; h < DDS_HANDS; h++)
      {
        aggHand[h][s] &= (~bitMapRank[k]);
        if (aggHand[h][s] > maxAgg)
        {
          maxAgg = aggHand[h][s];
          maxHand = h;
        }
      }
      if (maxAgg > 0)
      {
        posPoint->secondBest[s].hand = maxHand;
        posPoint->secondBest[s].rank = highestRank[aggHand[maxHand][s]];
      }
      else
      {
        posPoint->secondBest[s].hand = -1;
        posPoint->secondBest[s].rank = 0;
      }
    }
    else
    {
      posPoint->winner[s].hand = -1;
      posPoint->winner[s].rank = 0;
      posPoint->secondBest[s].hand = -1;
      posPoint->secondBest[s].rank = 0;
    }
  }

  for (int s = 0; s < DDS_SUITS; s++)
  {
    for (int h = 0; h < DDS_HANDS; h++)
      posPoint->length[h][s] =
        (unsigned char) counttable[posPoint->rankInSuit[h][s]];
  }

  // Clubs are implicit, for a given trick number.
  for (int h = 0; h < DDS_HANDS; h++)
  {
    posPoint->handDist[h] =
      (posPoint->length[h][0] << 8) |
      (posPoint->length[h][1] << 4) |
      (posPoint->length[h][2]     );
  }

  thrp->memUsed = thrp->transTable.MemoryInUse() +
    ThreadMemoryUsed();

#ifdef DDS_AB_STATS
  thrp->ABstats.Reset();
#endif
}


/* SH: Only visible in testing set-up, as not in dll.h */

void DDSidentify(char * s)
{
  sprintf(s, "DDS DLL\n-------\n");
#ifdef _WIN32
  s = strcat(s, "_WIN32\n");
#endif
#ifdef __CYGWIN__
  s = strcat(s, "__CYGWIN__\n");
#endif
#ifdef __MINGW32__
  s = strcat(s, "__MINGW32__\n");
#endif
#ifdef _MSC_VER
  s = strcat(s, "_MSC_VER\n");
#endif
#ifdef __cplusplus
  s = strcat(s, "__cplusplus\n");
#endif
#ifdef USES_DLLMAIN
  s = strcat(s, "USES_DLLMAIN\n");
#endif
#ifdef USES_CONSTRUCTOR
  s = strcat(s, "USES_CONSTRUCTOR\n");
#endif
#ifdef DDS_THREADS_SINGLE
  s = strcat(s, "No multi-threading\n");
#else
  #ifdef _OPENMP
    s = strcat(s, "OpenMP multi-threading\n");
  #else
    s = strcat(s, "WIN32 multi-threading\n");
  #endif
#endif

  char t[80];
  sprintf(t, "%d threads\n\n", noOfThreads);
  s = strcat(s, t);
}


void FreeThreadMem()
{
  for (int k = 0; k < noOfThreads; k++)
  {
    localVar[k].transTable.ResetMemory();
    localVar[k].memUsed = localVar[k].transTable.MemoryInUse() +
      ThreadMemoryUsed();
  }
}


void STDCALL FreeMemory()
{
  for (int k = 0; k < noOfThreads; k++)
  {
    localVar[k].transTable.ReturnAllMemory();
    localVar[k].memUsed = localVar[k].transTable.MemoryInUse() +
      ThreadMemoryUsed();
  }
}


double ConstantMemoryUsed()
{
  double memUsed =
    8192 * ( sizeof(int) // highestRank
           + sizeof(int) // counttable
	   + 15 * sizeof(char) // relRank
	   + 14 * sizeof(unsigned short int))
	   / (double) 1024.;

  return memUsed;
}


double ThreadMemoryUsed()
{
  double memUsed =
    8192 * sizeof(relRanksType)
    / (double) 1024.;

  return memUsed;
}


inline bool WinningMove(
  struct moveType * mvp1,
  struct moveType * mvp2,
  int trump)
{
  /* Return true if move 1 wins over move 2, with the assumption that
  move 2 is the presently winning card of the trick */

  if (mvp1->suit == mvp2->suit)
  {
    if ((mvp1->rank) > (mvp2->rank))
      return true;
    else
      return false;
  }
  else if ((mvp1->suit) == trump)
    return true;
  else
    return false;
}

