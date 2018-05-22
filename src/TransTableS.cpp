/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <iomanip>

#include "TransTableS.h"
#include "debug.h"


#define NSIZE 50000
#define WSIZE 50000
#define NINIT 60000
#define WINIT 170000
#define LSIZE 200 // Per trick and first hand


static bool _constantsSet = false;
static int TTlowestRank[8192];


TransTableS::TransTableS()
{
  if (! _constantsSet)
  {
    _constantsSet = true;
    TransTableS::SetConstants();
  }

  TTInUse = 0;
}


TransTableS::~TransTableS()
{
  TransTableS::ReturnAllMemory();
}


void TransTableS::SetConstants()
{
  unsigned int topBitRank = 1;
  TTlowestRank[0] = 15; // Void

  for (unsigned ind = 1; ind < 8192; ind++)
  {
    if (ind >= (topBitRank + topBitRank)) /* Next top bit */
      topBitRank <<= 1;

    TTlowestRank[ind] = TTlowestRank[ind ^ topBitRank] - 1;
  }

}


void TransTableS::Init(const int handLookup[][15])
{
  unsigned int topBitRank = 1;
  unsigned int topBitNo = 2;

  for (int s = 0; s < DDS_SUITS; s++)
  {
    aggp[0].aggrRanks[s] = 0;
    aggp[0].winMask[s] = 0;
  }

  for (unsigned int ind = 1; ind < 8192; ind++)
  {
    if (ind >= (topBitRank + topBitRank))
    {
      /* Next top bit */
      topBitRank <<= 1;
      topBitNo++;
    }
    aggp[ind] = aggp[ind ^ topBitRank];

    for (int s = 0; s < 4; s++)
    {
      aggp[ind].aggrRanks[s] =
        (aggp[ind].aggrRanks[s] >> 2) |
        (handLookup[s][topBitNo] << 24);

      aggp[ind].winMask[s] =
        (aggp[ind].winMask[s] >> 2) | (3 << 24);
    }
  }

  resetText.resize(TT_RESET_SIZE);
  resetText[TT_RESET_UNKNOWN] = "Unknown reason";
  resetText[TT_RESET_TOO_MANY_NODES] = "Too many nodes";
  resetText[TT_RESET_NEW_DEAL] = "New deal";
  resetText[TT_RESET_NEW_TRUMP] = "New trump";
  resetText[TT_RESET_MEMORY_EXHAUSTED] = "Memory exhausted";
  resetText[TT_RESET_FREE_MEMORY] = "Free thread memory";

  return;
}


void TransTableS::SetMemoryDefault(const int megabytes)
{
  UNUSED(megabytes);
}


void TransTableS::SetMemoryMaximum(const int megabytes)
{
  maxmem = static_cast<unsigned long long>(1000000 * megabytes);
}


void TransTableS::MakeTT()
{
  int i;

  if (!TTInUse)
  {
    TTInUse = 1;

    summem = (WINIT + 1) * sizeof(winCardType) +
             (NINIT + 1) * sizeof(nodeCardsType) +
             (LSIZE + 1) * 52 * sizeof(posSearchTypeSmall);
    wmem = static_cast<int>((WSIZE + 1) * sizeof(winCardType));
    nmem = static_cast<int>((NSIZE + 1) * sizeof(nodeCardsType));

    maxIndex = static_cast<int>(
      (maxmem - summem) / ((WSIZE + 1) * sizeof(winCardType)));

    pw = static_cast<winCardType **>(calloc(static_cast<unsigned int>(maxIndex + 1), sizeof(winCardType *)));
    if (pw == NULL)
      exit(1);

    pn = static_cast<nodeCardsType **>(calloc(static_cast<unsigned int>(maxIndex + 1), sizeof(nodeCardsType *)));
    if (pn == NULL)
      exit(1);

    for (int k = 1; k <= 13; k++)
      for (int h = 0; h < DDS_HANDS; h++)
      {
        pl[k][h] = static_cast
                   <posSearchTypeSmall **>(calloc(static_cast<unsigned int>(maxIndex + 1),
                                             sizeof(posSearchTypeSmall *)));
        if (pl[k][h] == NULL)
          exit(1);
      }

    for (i = 0; i <= maxIndex; i++)
    {
      if (pw[i])
        free(pw[i]);
      pw[i] = NULL;
    }

    for (i = 0; i <= maxIndex; i++)
    {
      if (pn[i])
        free(pn[i]);
      pn[i] = NULL;
    }

    for (int k = 1; k <= 13; k++)
    {
      for (int h = 0; h < DDS_HANDS; h++)
      {
        for (i = 0; i <= maxIndex; i++)
        {
          if (pl[k][h][i])
            free(pl[k][h][i]);
          pl[k][h][i] = NULL;
        }
      }
    }

    pw[0] = static_cast<winCardType *>(calloc(WINIT + 1, sizeof(winCardType)));
    if (pw[0] == NULL)
      exit(1);

    pn[0] = static_cast<nodeCardsType *>(calloc(NINIT + 1, sizeof(nodeCardsType)));
    if (pn[0] == NULL)
      exit(1);

    for (int k = 1; k <= 13; k++)
      for (int h = 0; h < DDS_HANDS; h++)
      {
        pl[k][h][0] = static_cast<posSearchTypeSmall *>(calloc((LSIZE + 1),
                      sizeof(posSearchTypeSmall)));
        if (pl[k][h][0] == NULL)
          exit(1);
      }

    aggp = static_cast<ttAggrType *>(calloc(8192, sizeof(ttAggrType)));
    if (aggp == NULL)
      exit(1);

    InitTT();

    for (int k = 1; k <= 13; k++)
      aggrLenSets[k] = 0;
    statsResets.noOfResets = 0;
    for (int k = 0; k <= 5; k++)
      statsResets.aggrResets[k] = 0;

  }

  return;
}


void TransTableS::Wipe()
{
  int m;

  for (m = 1; m <= wcount; m++)
  {
    if (pw[m])
      free(pw[m]);
    pw[m] = NULL;
  }
  for (m = 1; m <= ncount; m++)
  {
    if (pn[m])
      free(pn[m]);
    pn[m] = NULL;
  }

  for (int k = 1; k <= 13; k++)
  {
    for (int h = 0; h < DDS_HANDS; h++)
    {
      for (m = 1; m <= lcount[k][h]; m++)
      {
        if (pl[k][h][m])
          free(pl[k][h][m]);
        pl[k][h][m] = NULL;
      }
    }
  }

  allocmem = summem;

  return;
}



void TransTableS::InitTT()
{
  winSetSizeLimit = WINIT;
  nodeSetSizeLimit = NINIT;
  allocmem = (WINIT + 1) * sizeof(winCardType);
  allocmem += (NINIT + 1) * sizeof(nodeCardsType);
  allocmem += (LSIZE + 1) * 52 * sizeof(posSearchTypeSmall);
  winCards = pw[0];
  nodeCards = pn[0];
  wcount = 0;
  ncount = 0;

  nodeSetSize = 0;
  winSetSize = 0;

  clearTTflag = false;
  windex = -1;

  for (int k = 1; k <= 13; k++)
    for (int h = 0; h < DDS_HANDS; h++)
    {
      posSearch[k][h] = pl[k][h][0];
      lenSetInd[k][h] = 0;
      lcount[k][h] = 0;
    }
}


void TransTableS::ResetMemory(const TTresetReason reason)
{
  Wipe();

  InitTT();

  for (int k = 1; k <= 13; k++)
  {
    for (int h = 0; h < DDS_HANDS; h++)
    {
      rootnp[k][h] = &(posSearch[k][h][0]);
      posSearch[k][h][0].suitLengths = 0;
      posSearch[k][h][0].posSearchPoint = NULL;
      posSearch[k][h][0].left = NULL;
      posSearch[k][h][0].right = NULL;

      lenSetInd[k][h] = 1;
    }
  }

#if defined(DDS_TT_STATS)
  statsResets.noOfResets++;
  statsResets.aggrResets[reason]++;
#else
  UNUSED(reason);
#endif

  return;
}

void TransTableS::ReturnAllMemory()
{

  if (!TTInUse)
    return;
  TTInUse = 0;

  Wipe();

  if (pw[0])
    free(pw[0]);
  pw[0] = NULL;

  if (pn[0])
    free(pn[0]);
  pn[0] = NULL;

  for (int k = 1; k <= 13; k++)
  {
    for (int h = 0; h < DDS_HANDS; h++)
    {
      if (pl[k][h][0])
        free(pl[k][h][0]);
      pl[k][h][0] = NULL;
    }
  }

  if (pw)
    free(pw);
  pw = NULL;

  if (pn)
    free(pn);
  pn = NULL;

  if (aggp)
    free(aggp);
  aggp = NULL;

  return;
}


double TransTableS::MemoryInUse() const
{
  int ttMem = static_cast<int>(allocmem);
  int aggrMem = 8192 * static_cast<int>(sizeof(ttAggrType));
  return (ttMem + aggrMem) / static_cast<double>(1024.);
}


nodeCardsType const * TransTableS::Lookup(
  const int trick,
  const int hand,
  const unsigned short aggrTarget[],
  const int handDist[],
  const int limit,
  bool& lowerFlag)
{
  bool res;
  posSearchTypeSmall * pp;
  int orderSet[DDS_SUITS];
  nodeCardsType const * cardsP;

  suitLengths[trick] =
    (static_cast<long long>(handDist[0]) << 36) |
    (static_cast<long long>(handDist[1]) << 24) |
    (static_cast<long long>(handDist[2]) << 12) |
    (static_cast<long long>(handDist[3]));

  pp = SearchLenAndInsert(rootnp[trick][hand],
    suitLengths[trick], false, trick, hand, res);

  /* Find node that fits the suit lengths */
  if ((pp != NULL) && res)
  {
    for (int ss = 0; ss < DDS_SUITS; ss++)
    {
      orderSet[ss] =
        aggp[aggrTarget[ss]].aggrRanks[ss];
    }

    if (pp->posSearchPoint == NULL)
      cardsP = NULL;
    else
    {
      cardsP = FindSOP(orderSet, limit, pp->posSearchPoint, lowerFlag);

      if (cardsP == NULL)
        return cardsP;
    }
  }
  else
  {
    cardsP = NULL;
  }
  return cardsP;
}


void TransTableS::Add(
  const int tricks,
  const int hand,
  const unsigned short aggrTarget[],
  const unsigned short ourWinRanks[],
  const nodeCardsType& first,
  const bool flag)
{
  BuildSOP(ourWinRanks, aggrTarget, first, suitLengths[tricks],
           tricks, hand, flag);

  if (clearTTflag)
    ResetMemory(TT_RESET_MEMORY_EXHAUSTED);

  return;
}


void TransTableS::AddWinSet()
{
  if (clearTTflag)
  {
    windex++;
    winSetSize = windex;
    winCards = &(temp_win[windex]);
  }
  else if (winSetSize >= winSetSizeLimit)
  {
    /* The memory chunk for the winCards structure will be exceeded. */
    if (((allocmem + static_cast<unsigned long long>(wmem)) > maxmem) || (wcount >= maxIndex) ||
        (winSetSize > SIMILARMAXWINNODES))
    {
      /* Already allocated memory plus needed allocation overshot maxmem */
      windex++;
      winSetSize = windex;
      clearTTflag = true;
      winCards = &(temp_win[windex]);
    }
    else
    {
      wcount++;
      winSetSizeLimit = WSIZE;
      pw[wcount] =
        static_cast<winCardType *>(malloc((WSIZE + 1) * sizeof(winCardType)));
      if (pw[wcount] == NULL)
      {
        clearTTflag = true;
        windex++;
        winSetSize = windex;
        winCards = &(temp_win[windex]);
      }
      else
      {
        allocmem += (WSIZE + 1) * sizeof(winCardType);
        winSetSize = 0;
        winCards = pw[wcount];
      }
    }
  }
  else
    winSetSize++;
  return;
}

void TransTableS::AddNodeSet()
{
  if (nodeSetSize >= nodeSetSizeLimit)
  {
    /* The memory chunk for the nodeCards structure will be exceeded. */
    if (((allocmem + static_cast<unsigned long long>(nmem)) > maxmem) || (ncount >= maxIndex))
    {
      /* Already allocated memory plus needed allocation overshot maxmem */
      clearTTflag = true;
    }
    else
    {
      ncount++;
      nodeSetSizeLimit = NSIZE;
      pn[ncount] =
        static_cast<nodeCardsType *>(malloc((NSIZE + 1) * sizeof(nodeCardsType)));
      if (pn[ncount] == NULL)
      {
        clearTTflag = true;
      }
      else
      {
        allocmem += (NSIZE + 1) * sizeof(nodeCardsType);
        nodeSetSize = 0;
        nodeCards = pn[ncount];
      }
    }
  }
  else
    nodeSetSize++;
  return;
}

void TransTableS::AddLenSet(
  const int trick, 
  const int firstHand)
{
  if (lenSetInd[trick][firstHand] < LSIZE)
  {
    lenSetInd[trick][firstHand]++;
#if defined(DDS_TT_STATS)
    aggrLenSets[trick]++;
#endif
    return;
  }

  // The memory chunk for the posSearchTypeSmall structure 
  // will be exceeded.

  const int incr = (LSIZE+1) * sizeof(posSearchTypeSmall);

  if ((allocmem + incr > maxmem) || (lcount[trick][firstHand] >= maxIndex))
  {
    // Already allocated memory plus needed allocation overshot maxmem.
    clearTTflag = true;
    return;
  }

  // Obtain another memory chunk LSIZE.

  lcount[trick][firstHand]++;

  pl[trick][firstHand][lcount[trick][firstHand]] =
    static_cast<posSearchTypeSmall *>(malloc(incr));

  if (pl[trick][firstHand][lcount[trick][firstHand]] == NULL)
  {
    clearTTflag = true;
    return;
  }

  allocmem += incr;
  lenSetInd[trick][firstHand] = 0;
  posSearch[trick][firstHand] =
    pl[trick][firstHand][lcount[trick][firstHand]];
#if defined(DDS_TT_STATS)
  aggrLenSets[trick]++;
#endif
}


void TransTableS::BuildSOP(
  const unsigned short ourWinRanks[DDS_SUITS],
  const unsigned short aggrArg[DDS_SUITS],
  const nodeCardsType& first,
  const long long lengths,
  const int tricks,
  const int firstHand,
  const bool flag)
{
  int winMask[DDS_SUITS];
  int winOrderSet[DDS_SUITS];
  char low[DDS_SUITS];

  for (int ss = 0; ss < DDS_SUITS; ss++)
  {
    int w = ourWinRanks[ss];
    if (w == 0)
    {
      winMask[ss] = 0;
      winOrderSet[ss] = 0;
      low[ss] = 15;
    }
    else
    {
      w = w & (-w);       /* Only lowest win */
      const unsigned short temp = 
        static_cast<unsigned short>(aggrArg[ss] & (-w));

      winMask[ss] = aggp[temp].winMask[ss];
      winOrderSet[ss] = aggp[temp].aggrRanks[ss];
      low[ss] = static_cast<char>(TTlowestRank[temp]);
    }
  }

  bool res;
  posSearchTypeSmall * np = SearchLenAndInsert(
    rootnp[tricks][firstHand], lengths, true, tricks, firstHand, res);

  nodeCardsType * cardsP = BuildPath(
    winMask, 
    winOrderSet,
    static_cast<int>(first.ubound), 
    static_cast<int>(first.lbound),
    static_cast<char>(first.bestMoveSuit), 
    static_cast<char>(first.bestMoveRank),
    np, 
    res);

  if (res)
  {
    cardsP->ubound = static_cast<char>(first.ubound);
    cardsP->lbound = static_cast<char>(first.lbound);

    if (flag)
    {
      cardsP->bestMoveSuit = static_cast<char>(first.bestMoveSuit);
      cardsP->bestMoveRank = static_cast<char>(first.bestMoveRank);
    }
    else
    {
      cardsP->bestMoveSuit = 0;
      cardsP->bestMoveRank = 0;
    }

    for (int k = 0; k < DDS_SUITS; k++)
      cardsP->leastWin[k] = 15 - low[k];
  }
}


nodeCardsType * TransTableS::BuildPath(
  const int winMask[],
  const int winOrderSet[],
  const int ubound,
  const int lbound,
  const char bestMoveSuit,
  const char bestMoveRank,
  posSearchTypeSmall * nodep,
  bool& result)
{
  /* If result is TRUE, a new SOP has been created and BuildPath returns a
  pointer to it. If result is FALSE, an existing SOP is used and BuildPath
  returns a pointer to the SOP */

  bool found;
  winCardType * np, *p2, *nprev;
  nodeCardsType *p;

  np = nodep->posSearchPoint;
  nprev = NULL;
  int suit = 0;

  /* If winning node has a card that equals the next winning card deduced
  from the position, then there already exists a (partial) path */

  if (np == NULL)
  {
    /* There is no winning list created yet */
    /* Create winning nodes */
    p2 = &(winCards[winSetSize]);
    AddWinSet();
    p2->next = NULL;
    p2->nextWin = NULL;
    p2->prevWin = NULL;
    nodep->posSearchPoint = p2;
    p2->winMask = winMask[suit];
    p2->orderSet = winOrderSet[suit];
    p2->first = NULL;
    np = p2;           /* Latest winning node */
    suit++;
    while (suit < DDS_SUITS)
    {
      p2 = &(winCards[winSetSize]);
      AddWinSet();
      np->nextWin = p2;
      p2->prevWin = np;
      p2->next = NULL;
      p2->nextWin = NULL;
      p2->winMask = winMask[suit];
      p2->orderSet = winOrderSet[suit];
      p2->first = NULL;
      np = p2;         /* Latest winning node */
      suit++;
    }
    p = &(nodeCards[nodeSetSize]);
    AddNodeSet();
    np->first = p;
    result = true;
    return p;
  }
  else
  {
    /* Winning list exists */
    while (1)
    {
      /* Find all winning nodes that correspond to current position */
      found = false;
      while (1)      /* Find node amongst alternatives */
      {
        if ((np->winMask == winMask[suit]) &&
            (np->orderSet == winOrderSet[suit]))
        {
          /* Part of path found */
          found = true;
          nprev = np;
          break;
        }
        if (np->next != NULL)
          np = np->next;
        else
          break;
      }
      if (found)
      {
        suit++;
        if (suit >= DDS_SUITS)
        {
          result = false;
          return UpdateSOP(ubound, lbound, bestMoveSuit, bestMoveRank,
            np->first);
        }
        else
        {
          np = np->nextWin;       /* Find next winning node  */
          continue;
        }
      }
      else
        break;                    /* Node was not found */
    }               /* End outer while */

    /* Create additional node, coupled to existing node(s) */
    p2 = &(winCards[winSetSize]);
    AddWinSet();
    p2->prevWin = nprev;
    if (nprev != NULL)
    {
      p2->next = nprev->nextWin;
      nprev->nextWin = p2;
    }
    else
    {
      p2->next = nodep->posSearchPoint;
      nodep->posSearchPoint = p2;
    }
    p2->nextWin = NULL;
    p2->winMask = winMask[suit];
    p2->orderSet = winOrderSet[suit];
    p2->first = NULL;
    np = p2;          /* Latest winning node */
    suit++;

    /* Rest of path must be created */
    while (suit < 4)
    {
      p2 = &(winCards[winSetSize]);
      AddWinSet();
      np->nextWin = p2;
      p2->prevWin = np;
      p2->next = NULL;
      p2->winMask = winMask[suit];
      p2->orderSet = winOrderSet[suit];
      p2->first = NULL;
      p2->nextWin = NULL;
      np = p2;         /* Latest winning node */
      suit++;
    }

    /* All winning nodes in SOP have been traversed and new nodes created */
    p = &(nodeCards[nodeSetSize]);
    AddNodeSet();
    np->first = p;
    result = true;
    return p;
  }
}

TransTableS::posSearchTypeSmall * TransTableS::SearchLenAndInsert(
  posSearchTypeSmall * rootp,
  const long long key,
  const bool insertNode,
  const int trick,
  const int firstHand,
  bool& result)
{
  /* Search for node which matches with the suit length combination
  given by parameter key. If no such node is found, NULL is
  returned if parameter insertNode is FALSE, otherwise a new
  node is inserted with suitLengths set to key, the pointer to
  this node is returned.
  The algorithm used is defined in Knuth "The art of computer
  programming", vol.3 "Sorting and searching", 6.2.2 Algorithm T,
  page 424. */

  posSearchTypeSmall * np, *p, *sp;

  sp = NULL;
  if (insertNode)
    sp = &(posSearch[trick][firstHand][lenSetInd[trick][firstHand]]);

  np = rootp;
  while (1)
  {
    if (key == np->suitLengths)
    {
      result = true;
      return np;
    }
    else if (key < np->suitLengths)
    {
      if (np->left != NULL)
        np = np->left;
      else if (insertNode)
      {
        p = sp;
        AddLenSet(trick, firstHand);
        np->left = p;
        p->posSearchPoint = NULL;
        p->suitLengths = key;
        p->left = NULL;
        p->right = NULL;
        result = true;
        return p;
      }
      else
      {
        result = false;
        return NULL;
      }
    }
    else        /* key > suitLengths */
    {
      if (np->right != NULL)
        np = np->right;
      else if (insertNode)
      {
        p = sp;
        AddLenSet(trick, firstHand);
        np->right = p;
        p->posSearchPoint = NULL;
        p->suitLengths = key;
        p->left = NULL;
        p->right = NULL;
        result = true;
        return p;
      }
      else
      {
        result = false;
        return NULL;
      }
    }
  }
}


nodeCardsType * TransTableS::UpdateSOP(
  int ubound,
  int lbound,
  char bestMoveSuit,
  char bestMoveRank,
  nodeCardsType * nodep)
{
  /* Update SOP node with new values for upper and lower
  bounds. */
  if (lbound > nodep->lbound)
    nodep->lbound = static_cast<char>(lbound);
  if (ubound < nodep->ubound)
    nodep->ubound = static_cast<char>(ubound);

  nodep->bestMoveSuit = bestMoveSuit;
  nodep->bestMoveRank = bestMoveRank;

  return nodep;
}


nodeCardsType const * TransTableS::FindSOP(
  const int orderSet[],
  const int limit,
  winCardType * nodeP,
  bool& lowerFlag)
{
  winCardType * np;

  np = nodeP;
  int s = 0;

  while (np)
  {
    if ((np->winMask & orderSet[s]) == np->orderSet)
    {
      /* Winning rank set fits position */
      if (s != 3)
      {
        np = np->nextWin;
        s++;
        continue;
      }

      if (np->first->lbound > limit)
      {
        lowerFlag = true;
        return np->first;
      }
      else if (np->first->ubound <= limit)
      {
        lowerFlag = false;
        return np->first;
      }
    }

    while (np->next == NULL)
    {
      np = np->prevWin;
      s--;
      if (np == NULL) /* Previous node is header node? */
        return NULL;
    }
    np = np->next;
  }
  return NULL;
}


void TransTableS::PrintNodeStats(ofstream& fout) const
{
  fout << "Report of generated PosSearch nodes per trick level.\n";
  fout << "Trick level 13 is highest level with all 52 cards.\n";
  fout << string(51, '-') << "\n";

  fout << setw(5) << "Trick" << 
    setw(14) << right << "Created nodes" << "\n";

  for (int k = 13; k > 0; k--)
    fout << setw(5) << k << setw(14) << aggrLenSets[k-1] << "\n";

  fout << endl;
}


void TransTableS::PrintResetStats(ofstream& fout) const
{
  fout << "Total no. of resets: " << statsResets.noOfResets << "\n" << endl;

  fout << setw(18) << left << "Reason" << 
    setw(6) << right << "Count" << "\n";

  for (unsigned k = 0; k < TT_RESET_SIZE; k++)
    fout << setw(18) << left << resetText[k] <<
      setw(6) << right << statsResets.aggrResets[k] << "\n";
}

