/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "Scheduler.h"


Scheduler::Scheduler()
{
  // This can be HCP, for instance. Currently it is close to
  // 6 - 4 - 2 - 1 - 0.5 for A-K-Q-J-T, but with 6.5 for the ace
  // in order to make the sum come out to 28, an even number, so
  // that the average number is an integer.

  for (int i = 0; i < 8192; i++)
  {
    highCards[i] = 0;

    if (i & (1 << 12)) highCards[i] += 13;
    if (i & (1 << 11)) highCards[i] += 8;
    if (i & (1 << 10)) highCards[i] += 4;
    if (i & (1 << 9)) highCards[i] += 2;
    if (i & (1 << 8)) highCards[i] += 1;
  }

  numHands = 0;

#ifdef DDS_SCHEDULER
  Scheduler::InitTimes();

  for (int i = 0; i < 10000; i++)
  {
    timeHist[i] = 0;
    timeHistNT[i] = 0;
    timeHistSuit[i] = 0;
  }

  sprintf(fname, "");
  fp = stdout;
#endif

#if defined(_OPENMP) && !defined(DDS_THREADS_SINGLE)
  omp_init_lock(&lock);
#endif
}


void Scheduler::SetFile(char * ourFname)
{
#ifdef DDS_SCHEDULER
  if (strlen(ourFname) > 80)
    return;

  if (fp != stdout) // Already set
    return;
  strncpy(fname, ourFname, strlen(ourFname));

  fp = fopen(fname, "w");
  if (! fp)
    fp = stdout;
#else
  UNUSED(ourFname);
#endif
}


#ifdef DDS_SCHEDULER
void Scheduler::InitTimes()
{
  for (int s = 0; s < 2; s++)
  {
    timeStrain[s].cum = 0;
    timeStrain[s].cumsq = 0;
    timeStrain[s].number = 0;

    timeGroupActualStrain[s].cum = 0;
    timeGroupActualStrain[s].cumsq = 0;
    timeGroupActualStrain[s].number = 0;

    timeGroupPredStrain[s].cum = 0;
    timeGroupPredStrain[s].cumsq = 0;
    timeGroupPredStrain[s].number = 0;

    timeGroupDiffStrain[s].cum = 0;
    timeGroupDiffStrain[s].cumsq = 0;
    timeGroupDiffStrain[s].number = 0;
  }

  for (int s = 0; s < 16; s++)
  {
    timeRepeat[s].cum = 0;
    timeRepeat[s].cumsq = 0;
    timeRepeat[s].number = 0;
  }

  for (int s = 0; s < 60; s++)
  {
    timeDepth[s].cum = 0;
    timeDepth[s].cumsq = 0;
    timeDepth[s].number = 0;
  }

  for (int s = 0; s < 60; s++)
  {
    timeStrength[s].cum = 0;
    timeStrength[s].cumsq = 0;
    timeStrength[s].number = 0;
  }

  for (int s = 0; s < 100; s++)
  {
    timeFanout[s].cum = 0;
    timeFanout[s].cumsq = 0;
    timeFanout[s].number = 0;
  }

  for (int s = 0; s < MAXNOOFTHREADS; s++)
  {
    timeThread[s].cum = 0;
    timeThread[s].cumsq = 0;
    timeThread[s].number = 0;
  }

  blockMax = 0;
  timeBlock = 0;
}
#endif


Scheduler::~Scheduler()
{
#ifdef DDS_SCHEDULER
  if (fp != stdout && fp != nullptr)
    fclose(fp);
#endif

#if defined(_OPENMP) && !defined(DDS_THREADS_SINGLE)
  omp_destroy_lock(&lock);
#endif
}


void Scheduler::Reset()
{
  for (int b = 0; b < MAXNOOFBOARDS; b++)
    hands[b].next = -1;

  numGroups = 0;
  extraGroups = 0;

  // One extra for NT, one extra for splitting collisions.
  for (int strain = 0; strain < DDS_SUITS + 2; strain++)
    for (int key = 0; key < HASH_MAX; key++)
      list[strain][key].first = -1;


  for (int t = 0; t < MAXNOOFTHREADS; t++)
  {
    threadGroup[t] = -1;
    threadCurrGroup[t] = -1;
  }

  currGroup = -1;
}


void Scheduler::RegisterTraceDepth(
  playTracesBin * plp,
  int number)
{
  // This is only used for traces, so it is entered separately.

#ifdef DDS_SCHEDULER
  for (int b = 0; b < number; b++)
    hands[b].depth = plp->plays[b].number;
#else
  UNUSED(plp);
  UNUSED(number);
#endif
}


void Scheduler::Register(
  boards * bop,
  int sortMode)
{
  Scheduler::Reset();

  numHands = bop->noOfBoards;

  // First split the hands according to strain and hash key.
  // This will lead to a few random collisions as well.

  Scheduler::MakeGroups(bop);

  // Then check whether groups with at least two elements are
  // homogeneous or whether they need to be split.

  Scheduler::FinetuneGroups();

  // Make predictions per group.

  if (sortMode == SCHEDULER_SOLVE)
    Scheduler::SortSolve();
  else if (sortMode == SCHEDULER_CALC)
    Scheduler::SortCalc();
  else if (sortMode == SCHEDULER_TRACE)
    Scheduler::SortTrace();
}


void Scheduler::MakeGroups(
  boards * bop)
{
  deal * dl;
  listType * lp;

  for (int b = 0; b < numHands; b++)
  {
    dl = &bop->deals[b];

    int strain = dl->trump;

    unsigned dlXor =
      dl->remainCards[0][0] ^
      dl->remainCards[1][1] ^
      dl->remainCards[2][2] ^
      dl->remainCards[3][3];

    int key = static_cast<int>(((dlXor >> 2) ^ (dlXor >> 6)) & 0x7f);

    hands[b].spareKey = static_cast<int>(
                          (dl->remainCards[1][0] << 17) ^
                          (dl->remainCards[2][1] << 11) ^
                          (dl->remainCards[3][2] << 5) ^
                          (dl->remainCards[0][3] >> 2));

    for (int h = 0; h < DDS_HANDS; h++)
      for (int s = 0; s < DDS_SUITS; s++)
        hands[b].remainCards[h][s] = dl->remainCards[h][s];

    hands[b].NTflag = (strain == 4 ? 1 : 0);
    hands[b].first = dl->first;
    hands[b].strain = strain;
    hands[b].fanout = Scheduler::Fanout(dl);
    // hands[b].strength = Scheduler::Strength(dl);

    lp = &list[strain][key];

    if (lp->first == -1)
    {
      lp->first = b;
      lp->last = b;
      lp->length = 1;

      group[numGroups].strain = strain;
      group[numGroups].hash = key;
      numGroups++;
    }
    else
    {
      int l = lp->last;
      hands[l].next = b;

      lp->last = b;
      lp->length++;
    }
  }
}


void Scheduler::FinetuneGroups()
{
  listType * lp;
  int strain, key, b1, b2;
  int numGroupsOrig = numGroups;

  for (int g = 0; g < numGroupsOrig; g++)
  {
    strain = group[g].strain;
    key = group[g].hash;

    lp = &list[strain][key];

    if (lp->length == 1)
      continue;

    else if (lp->length == 2)
    {
      // This happens quite often, so worth optimizing.

      b1 = lp->first;
      b2 = hands[lp->first].next;

      bool match = false;
      if (hands[b1].spareKey == hands[b2].spareKey)
      {
        // It is now extremely likely that it is a repeat hand,
        // but we have to be sure.
        match = true;
        for (int h = 0; h < DDS_HANDS && match; h++)
          for (int s = 0; s < DDS_SUITS && match; s++)
            if (hands[b1].remainCards[h][s] != hands[b2].remainCards[h][s])
              match = false;
      }

      if (match)
        continue;

      // Leave the first hand in place.
      hands[lp->first].next = -1;
      lp->last = lp->first;
      lp->length = 1;

      // Move the second hand to the special list.
      lp = &list[5][extraGroups];

      lp->first = b2;
      lp->last = b2;
      lp->length = 1;

      group[numGroups].strain = 5;
      group[numGroups].hash = extraGroups;

      numGroups++;
      extraGroups++;
    }

    else
    {
      // This is the general case. The comparison is not quite
      // as thorough here, but it's better than above and it uses
      // a different hand.

      sortType st;
      sortLen = lp->length;
      int index = lp->first;

      for (int i = 0; i < sortLen; i++)
      {
        sortList[i].number = index;
        sortList[i].value = hands[index].spareKey;

        index = hands[index].next;
      }

      // Sort the list heuristically by spareKey value.

      for (int i = 1; i < sortLen; i++)
      {
        st = sortList[i];
        int j = i;
        for (; j && st.value > sortList[j - 1].value; --j)
          sortList[j] = sortList[j - 1];
        sortList[j] = st;
      }

      // First group stays where it is, but shorter and rejigged.
      // From here on, hand comparisons are completely rigorous.
      // We might miss duplicates, but we won't let different
      // hands through as belonging to the same group.

      int l = 0;
      while (l < sortLen-1 && 
        Scheduler::SameHand(sortList[l].number, sortList[l+1].number))
        l++;

      if (l == sortLen-1)
        continue;

      lp->first = sortList[0].number;
      lp->last = sortList[l].number;
      lp->length = l + 1;

      index = lp->first;

      for (int i = 0; i < l; i++)
      {
        hands[index].next = sortList[i + 1].number;
        index = hands[index].next;
      }

      hands[index].next = -1;

      // The rest is moved to special groups.
      l++;

      while (l < sortLen)
      {
        if (Scheduler::SameHand(sortList[l].number, sortList[l-1].number))
        {
          // Same group
          int nOld = sortList[l - 1].number;
          int nNew = sortList[l].number;
          hands[nOld].next = nNew;
          hands[nNew].next = -1;

          lp->last = nNew;
          lp->length++;
        }
        else
        {
          // New group
          int n = sortList[l].number;
          hands[n].next = -1;

          lp = &list[5][extraGroups];
          lp->first = n;
          lp->last = n;
          lp->length = 1;

          group[numGroups].strain = 5;
          group[numGroups].hash = extraGroups;

          numGroups++;
          extraGroups++;
        }
        l++;
      }
    }
  }
}


bool Scheduler::SameHand(
  int hno1,
  int hno2)
{
  for (int h = 0; h < DDS_HANDS; h++)
    for (int s = 0; s < DDS_SUITS; s++)
      if (hands[hno1].remainCards[h][s] != hands[hno2].remainCards[h][s])
        return false;

  return true;
}


// These are specific times from a 12-core PC. The hope is
// that they scale somewhat proportionally to other cases.
// The strength parameter is currently not used.

int SORT_SOLVE_TIMES[2][8] =
{
  { 284000,  91000, 37000, 23000, 17000, 15000, 13000, 4000 },
  { 388000, 140000, 60000, 40000, 30000, 23000, 18000, 6000 },
};

#define SORT_SOLVE_STRENGTH_CUTOFF 0

double SORT_SOLVE_STRENGTH[2][3] =
{
  { 1.525, 1.810, 0.0285 },
  { 1.585, 1.940, 0.0354 }
};

// Lower end of linear, upper end of linear, slope of linear,
// exponential start, coefficient.

double SORT_SOLVE_FANOUT[2][5] =
{
  { 30., 50., 0.07577, 1.515, 12. },
  { 30., 50., 0.08144, 1.629, 12. }
};

void Scheduler::SortSolve()
{
  listType * lp;
  handType * hp;
  int strain, key, index;

  for (int g = 0; g < numGroups; g++)
  {
    strain = group[g].strain;
    key = group[g].hash;
    lp = &list[strain][key];
    index = lp->first;
    hp = &hands[index];

    // Taking into account repeat times saves 1-2%.

    int repeatNo = 0;
    int firstPrev = -1;
    group[g].pred = 0;
    do
    {
      // Skip complete duplicates, as we won't solve them again.
      if (hands[index].first != firstPrev)
      {
        group[g].pred += SORT_SOLVE_TIMES[hp->NTflag][repeatNo];
        if (repeatNo < 7)
          repeatNo++;
        firstPrev = hands[index].first;
      }

      index = hands[index].next;
    }
    while (index != -1);

    // Taking into account fanout saves 4-6%.

    int fanout = hp->fanout;
    double * slist = SORT_SOLVE_FANOUT[hp->NTflag];
    double fanoutFactor;

    if (fanout < slist[0])
      fanoutFactor = 0.; // A bit extreme...
    else if (fanout < slist[1])
      fanoutFactor = slist[2] * (fanout - slist[0]);
    else
      fanoutFactor = slist[3] * exp( (fanout - slist[1]) / slist[4] );

    group[g].pred = static_cast<int>(
      (fanoutFactor * static_cast<double>(group[g].pred)));
  }

  // Sort groups using merge sort.
  groupType gp;
  for (int g = 0; g < numGroups; g++)
  {
    gp = group[g];
    int j = g;
    for (; j && gp.pred > group[j - 1].pred; --j)
      group[j] = group[j - 1];
    group[j] = gp;
  }
}


// For calc there is no repeat overhead ever, as this is always
// a direct copy.

// Lower end of linear, upper end of linear, slope of linear,
// exponential start, coefficient.

double SORT_CALC_FANOUT[2][5] =
{
  { 30., 50., 0.07812, 1.563, 13. },
  { 30., 50., 0.07739, 1.548, 12. }
};


void Scheduler::SortCalc()
{
  listType * lp;
  handType * hp;
  int strain, key, index;

  for (int g = 0; g < numGroups; g++)
  {
    strain = group[g].strain;
    key = group[g].hash;
    lp = &list[strain][key];
    index = lp->first;
    hp = &hands[index];

    // Taking into account repeat times saves 1-2%.

    group[g].pred = 272000;

    int fanout = hp->fanout;
    double * slist = SORT_CALC_FANOUT[hp->NTflag];
    double fanoutFactor;

    if (fanout < slist[0])
      fanoutFactor = 0.; // A bit extreme...
    else if (fanout < slist[1])
      fanoutFactor = slist[2] * (fanout - slist[0]);
    else
      fanoutFactor = slist[3] * exp( (fanout - slist[1]) / slist[4] );

    group[g].pred = static_cast<int>(
      (fanoutFactor * static_cast<double>(group[g].pred)));
  }

  // Sort groups using merge sort.
  groupType gp;
  for (int g = 0; g < numGroups; g++)
  {
    gp = group[g];
    int j = g;
    for (; j && gp.pred > group[j - 1].pred; --j)
      group[j] = group[j - 1];
    group[j] = gp;
  }
}


// These are specific times from a 12-core PC. The hope is
// that they scale somewhat proportionally to other cases.

int SORT_TRACE_TIMES[2][8] =
{
  { 157000, 47000, 26000, 18000, 16000, 14000, 10000,  6000 },
  { 205000, 87000, 45000, 36000, 32000, 28000, 24000, 20000 },
};

// Initial value for 0 and 1 cards
// Value up to 15 cards incl
// Slope between 16 and 48 incl
// Average for 49-52

double SORT_TRACE_DEPTH[2][4] =
{
  { 0.742, 0.411, 0.0414, 1.820 },
  { 0.669, 0.428, 0.0346, 1.606 }
};

// Lower end of linear, upper end of linear, slope of linear,
// exponential start, coefficient.

double SORT_TRACE_FANOUT[2][5] =
{
  { 30., 50., 0.07577, 1.515, 12. },
  { 30., 50., 0.08166, 1.633, 13. }
};

void Scheduler::SortTrace()
{
  listType * lp;
  handType * hp;
  int strain, key, index;

  for (int g = 0; g < numGroups; g++)
  {
    strain = group[g].strain;
    key = group[g].hash;
    lp = &list[strain][key];
    index = lp->first;
    hp = &hands[index];

    // Taking into account repeat times.

    int repeatNo = 0;
    int firstPrev = -1;
    group[g].pred = 0;
    do
    {
      // Skip complete duplicates, as we won't solve them again.
      if (hands[index].first != firstPrev)
      {
        group[g].pred += SORT_TRACE_TIMES[hp->NTflag][repeatNo];
        if (repeatNo < 7)
          repeatNo++;
        firstPrev = hands[index].first;
      }

      index = hands[index].next;
    }
    while (index != -1);

    double depthFactor;
    int depth = hp->depth;
    double * slist = SORT_TRACE_DEPTH[hp->NTflag];

    if (depth <= 1)
      depthFactor = slist[0];
    else if (depth <= 15)
      depthFactor = slist[1];
    else if (depth >= 49)
      depthFactor = slist[3];
    else
      depthFactor = slist[1] + (depth - 15) * slist[2];

    group[g].pred = static_cast<int>(
      (depthFactor * static_cast<double>(group[g].pred)));

    // Taking into account fanout.

    int fanout = hp->fanout;
    slist = SORT_TRACE_FANOUT[hp->NTflag];
    double fanoutFactor;

    if (fanout < slist[0])
      fanoutFactor = 0.; // A bit extreme...
    else if (fanout < slist[1])
      fanoutFactor = slist[2] * (fanout - slist[0]);
    else
      fanoutFactor = slist[3] * exp( (fanout - slist[1]) / slist[4] );

    group[g].pred = static_cast<int>(
      (fanoutFactor * static_cast<double>(group[g].pred)));
  }

  // Sort groups using merge sort.
  groupType gp;
  for (int g = 0; g < numGroups; g++)
  {
    gp = group[g];
    int j = g;
    for (; j && gp.pred > group[j - 1].pred; --j)
      group[j] = group[j - 1];
    group[j] = gp;
  }
}


int Scheduler::Strength(
  deal * dl)
{
  // If the strength in all suits is evenly split, then the
  // "strength" returned is close to 0. Maximum is 49.

  unsigned sp = (dl->remainCards[0][0] | dl->remainCards[2][0]) >> 2;
  unsigned he = (dl->remainCards[0][1] | dl->remainCards[2][1]) >> 2;
  unsigned di = (dl->remainCards[0][2] | dl->remainCards[2][2]) >> 2;
  unsigned cl = (dl->remainCards[0][3] | dl->remainCards[2][3]) >> 2;

  int hsp = highCards[sp];
  int hhe = highCards[he];
  int hdi = highCards[di];
  int hcl = highCards[cl];

  int dev = (hsp >= 14 ? hsp - 14 : 14 - hsp) +
            (hhe >= 14 ? hhe - 14 : 14 - hhe) +
            (hdi >= 14 ? hdi - 14 : 14 - hdi) +
            (hcl >= 14 ? hcl - 14 : 14 - hcl);

  if (dev >= 50) dev = 49;

  return dev;
}


int Scheduler::Fanout(
  deal * dl)
{
  // The fanout for a given suit and a given player is the number
  // of bit groups, so KT982 has 3 groups. In a given suit the
  // maximum number over all four players is 13.
  // A void counts as the sum of the other players' groups.

  int fanout = 0;
  int fanoutSuit, numVoids, c;

  for (int h = 0; h < DDS_HANDS; h++)
  {
    fanoutSuit = 0;
    numVoids = 0;
    for (int s = 0; s < DDS_SUITS; s++)
    {
      c = static_cast<int>(dl->remainCards[h][s] >> 2);
      fanoutSuit += groupData[c].lastGroup + 1;
      if (c == 0)
        numVoids++;
    }
    fanoutSuit += numVoids * fanoutSuit;
    fanout += fanoutSuit;
  }

  return fanout;
}


schedType Scheduler::GetNumber(
  int thrId)
{
  int g = threadGroup[thrId];
  listType * lp;
  schedType st;

  if (g == -1)
  {
    // Find a new group

    if (currGroup >= numGroups - 1)
    {
      // Out of groups. Just an optimization not to touch the
      // shared variable unnecessarily.
      st.number = -1;
      return st;
    }

#if (defined(_WIN32) || defined(__CYGWIN__)) && \
       !defined(_OPENMP) && !defined(DDS_THREADS_SINGLE)
    g = InterlockedIncrement(&currGroup);
#elif defined(_OPENMP) && !defined(DDS_THREADS_SINGLE)
    omp_set_lock(&lock);
    g = ++currGroup;
    omp_unset_lock(&lock);
#else
    g = ++currGroup;
#endif

    if (g >= numGroups)
    {
      // Out of groups. currGroup could have changed in the
      // meantime in another thread, so test again.

      st.number = -1;
      return st;
    }

    // A bit inelegant to duplicate this, but seems better than
    // the alternative, as threadGroup must get set to -1 in some
    // cases.
    threadGroup[thrId] = g;
    threadCurrGroup[thrId] = g;
    group[g].repeatNo = 0;
    group[g].actual = 0;
  }

  // Continue with existing or new group

  int strain = group[g].strain;
  int key = group[g].hash;

  lp = &list[strain][key];
  st.number = lp->first;
  lp->first = hands[lp->first].next;

  if (group[g].repeatNo == 0)
  {
    group[g].head = st.number;
    st.repeatOf = -1;

    // Only first-solve suited hands for statistics right now.
    hands[st.number].selectFlag =
      (hands[st.number].strain == 4 ? 1 : 0);
  }
  else
  {
    st.repeatOf = group[g].head;
    //hands[st.number].selectFlag = 0;

    if (hands[st.number].first == hands[st.repeatOf].first)
      hands[st.number].selectFlag = 0;
    else if (hands[st.number].strain == 4)
      hands[st.number].selectFlag = 1;
    else
      hands[st.number].selectFlag = 0;
  }

  hands[st.number].repeatNo = group[g].repeatNo++;

  threadToHand[thrId] = st.number;

  if (lp->first == -1)
    threadGroup[thrId] = -1;

  return st;
}


#ifdef DDS_SCHEDULER
void Scheduler::StartThreadTimer(int thrId)
{
#ifdef _WIN32
  QueryPerformanceCounter(&timeStart[thrId]);
#else
  gettimeofday(&timeStart[thrId], NULL);
#endif
}


void Scheduler::EndThreadTimer(int thrId)
{
#ifdef _WIN32
  QueryPerformanceCounter(&timeEnd[thrId]);
  int timeUser = (timeEnd [thrId].QuadPart -
                  timeStart[thrId].QuadPart);
#else
  gettimeofday(&timerListUser1[no], NULL);
  int timeUser = Scheduler::timeDiff(timeEnd[thrId], timeStart[thrId]);
#endif

  hands[ threadToHand[thrId] ].time = timeUser;
  hands[ threadToHand[thrId] ].thread = thrId;

  group[ threadCurrGroup[thrId] ].actual += timeUser;
}


void Scheduler::StartBlockTimer()
{
#ifdef _WIN32
  QueryPerformanceCounter(&blockStart);
#else
  gettimeofday(&blockStart, NULL);
#endif
}


void Scheduler::EndBlockTimer()
{
#ifdef _WIN32
  QueryPerformanceCounter(&blockEnd);
  int timeUser = (blockEnd .QuadPart -
                  blockStart.QuadPart);
#else
  gettimeofday(&blockEnd, NULL);
  int timeUser = Scheduler::timeDiff(blockEnd, blockStart);
#endif

  handType * hp;
  for (int b = 0; b < numHands; b++)
  {
    hp = &hands[b];
    int timeUser = hp->time;
    double timesq = (double) timeUser * (double) timeUser;

    if (hp->selectFlag)
    {
      timeStrain [ hp->NTflag ].number++;
      timeStrain [ hp->NTflag ].cum += timeUser;
      timeStrain [ hp->NTflag ].cumsq += timesq;

      timeRepeat [ hp->repeatNo ].number++;
      timeRepeat [ hp->repeatNo ].cum += timeUser;
      timeRepeat [ hp->repeatNo ].cumsq += timesq;

      timeDepth [ hp->depth ].number++;
      timeDepth [ hp->depth ].cum += timeUser;
      timeDepth [ hp->depth ].cumsq += timesq;

      timeStrength[ hp->strength ].number++;
      timeStrength[ hp->strength ].cum += timeUser;
      timeStrength[ hp->strength ].cumsq += timesq;

      timeFanout [ hp->fanout ].number++;
      timeFanout [ hp->fanout ].cum += timeUser;
      timeFanout [ hp->fanout ].cumsq += timesq;

      timeThread [ hp->thread ].number++;
      timeThread [ hp->thread ].cum += timeUser;
      timeThread [ hp->thread ].cumsq += timesq;
    }

    if (timeUser > blockMax)
      blockMax = timeUser;

    if (hp->repeatNo == 0)
    {
      int bin = timeUser / 1000;
      timeHist[bin]++;
      if (hp->NTflag)
        timeHistNT[bin]++;
      else
        timeHistSuit[bin]++;
    }
  }

  for (int g = 0; g < numGroups; g++)
  {
    int head = group[g].head;
    int NTflag = (hands[head].strain == 4 ? 1 : 0);

    timeGroupActualStrain[NTflag].number++;
    timeGroupActualStrain[NTflag].cum += group[g].actual;
    timeGroupActualStrain[NTflag].cumsq +=
      (double) group[g].actual * (double) group[g].actual;

    timeGroupPredStrain [NTflag].number++;
    timeGroupPredStrain [NTflag].cum += group[g].pred;
    timeGroupPredStrain [NTflag].cumsq +=
      group[g].pred * group[g].pred;

    double diff = group[g].actual - group[g].pred;

    timeGroupDiffStrain [NTflag].number++;
    timeGroupDiffStrain [NTflag].cum += diff;
    timeGroupDiffStrain [NTflag].cumsq += diff * diff;
  }

  timeBlock += timeUser;
  timeMax += blockMax;
  blockMax = 0;
}


void Scheduler::PrintTimingList(
  timeType * tp,
  int length,
  const char title[])
{
  bool empty = true;
  for (int no = 0; no < length && empty; no++)
  {
    if (tp[no].number)
      empty = false;
  }
  if (empty)
    return;

  fprintf(fp, "%s\n\n", title);
  fprintf(fp, "%5s %8s %12s %12s %12s %12s\n",
          "n", "Number", "Cum time", "Average", "Sdev", "Sdev/mu");

  long long sn = 0, st = 0;
  double sq = 0;

  for (int no = 0; no < length; no++)
  {
    if (tp[no].number == 0)
      continue;

    sn += tp[no].number;
    st += tp[no].cum;
    sq += tp[no].cumsq;

    double avg = (double) tp[no].cum / (double) tp[no].number;
    double arg = (tp[no].cumsq / (double) tp[no].number) -
                  (double) avg * (double) avg;
    double sdev = (arg >= 0. ? sqrt(arg) : 0.);

    fprintf(fp, "%5d %8d %12lld ",
            no,
            tp[no].number,
            tp[no].cum);
    fprintf(fp, "%12.0f %12.0f %12.2f\n", avg, sdev, sdev / avg);
  }

  if (sn)
  {
    double avg = (double) st / (double) sn;
    double arg = (sq / (double) sn) - (double) avg * (double) avg;
    double sdev = (arg >= 0. ? sqrt(arg) : 0.);
    fprintf(fp, " Avg %8lld %12lld ", sn, st);
    fprintf(fp, "%12.0f %12.0f %12.2f\n", avg, sdev, sdev / avg);
  }

  fprintf(fp, "\n");
}


void Scheduler::PrintTiming()
{
  Scheduler::PrintTimingList(timeStrain , 2, "Suit/NT");
  Scheduler::PrintTimingList(timeRepeat , 16, "Repeat number");
  Scheduler::PrintTimingList(timeDepth , 60, "Trace depth");
  Scheduler::PrintTimingList(timeStrength, 60, "Evenness");
  Scheduler::PrintTimingList(timeFanout , 100, "Fanout");
  Scheduler::PrintTimingList(timeThread , MAXNOOFTHREADS, "Threads");

  Scheduler::PrintTimingList(timeGroupActualStrain, 2,
                             "Group actual suit/NT");
  Scheduler::PrintTimingList(timeGroupPredStrain , 2,
                             "Group predicted suit/NT");
  Scheduler::PrintTimingList(timeGroupDiffStrain , 2,
                             "Group diff suit/NT");

#if 0
  for (int i = 0; i < 10000; i++)
    if (timeHist[i] || timeHistSuit[i] || timeHistNT[i])
      fprintf(fp, "%4d %8d %8d %8d\n",
              i, timeHist[i], timeHistSuit[i], timeHistNT[i]);
  fprintf(fp, "\n");
#endif

  if (timeBlock == 0)
    return;

  // Continuing problems with ld in long fprintf's...
  double avg = 100. * (double) timeMax / (double) timeBlock;
  fprintf(fp, "Largest hand %12lld ", timeMax);
  fprintf(fp, "%12lld ", timeBlock);
  fprintf(fp, "%5.2f%%\n\n", avg);
}


#ifndef _WIN32
int Scheduler::timeDiff(
  timeval x,
  timeval y)
{
  /* Elapsed time, x-y, in milliseconds */
  return 1000 * (x.tv_sec - y.tv_sec )
         + (x.tv_usec - y.tv_usec) / 1000;
}
#endif

#endif // DDS_SCHEDULER


int Scheduler::PredictedTime(
  deal * dl,
  int number)
{
  int trump = dl->trump;
  int NT = (trump == 4 ? 100 : 0);

  int dev1 = Scheduler::Strength(dl);

  int pred;
  if (NT)
  {
    if (dev1 >= 25)
      pred = 125000 - 2500 * dev1;
    else
      // This branch is not very accurate.
      pred = 200000 - 5500 * dev1;

    if (number >= 1)
      pred = static_cast<int>(1.25 * pred);

    if (number >= 2)
      pred = static_cast<int> (pred *
        (1.185 - 0.185 * exp( -(number - 1) / 6.0)));
  }
  else
  {
    pred = 125000 - 2500 * dev1;
    if (number >= 1)
      pred = static_cast<int>(1.2 * pred);

    if (number >= 2)
      pred = static_cast<int>(pred *
        (1.185 - 0.185 * exp( -(number - 1) / 5.5)));
  }

  return pred;
}


