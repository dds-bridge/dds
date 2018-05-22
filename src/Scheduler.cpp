/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#include <iomanip>
#include <sstream>
#include <math.h>

#include "Scheduler.h"


Scheduler::Scheduler()
{
  numThreads = 0;
  numHands = 0;

  Scheduler::InitHighCards();

#ifdef DDS_SCHEDULER
  Scheduler::InitTimes();
  for (int i = 0; i < 10000; i++)
  {
    timeHist[i] = 0;
    timeHistNT[i] = 0;
    timeHistSuit[i] = 0;
  }
#endif

  Scheduler::RegisterThreads(1);
}


void Scheduler::InitHighCards()
{
  // highCards[i] is a point value of a given suit holding i.
  // This can be HCP, for instance. Currently it is close to
  // 6 - 4 - 2 - 1 - 0.5 for A-K-Q-J-T, but with 6.5 for the ace
  // in order to make the sum come out to 28, an even number, so
  // that the average number is an integer.

  highCards.resize(1 << 13);
  const unsigned pA = 1 << 12;
  const unsigned pK = 1 << 11;
  const unsigned pQ = 1 << 10;
  const unsigned pJ = 1 << 9;
  const unsigned pT = 1 << 8;

  for (unsigned suit = 0; suit < (1 << 13); suit++)
  {
    int j = 0;
    if (suit & pA) j += 13;
    if (suit & pK) j += 8;
    if (suit & pQ) j += 4;
    if (suit & pJ) j += 2;
    if (suit & pT) j += 1;
    highCards[suit] = j;
  }
}


#ifdef DDS_SCHEDULER
void Scheduler::InitTimes()
{
  timeStrain.Init("Suit/NT", 2);
  timeRepeat.Init("Repeat number", 16);
  timeDepth.Init("Trace depth", 60);
  timeStrength.Init("Evenness", 60);
  timeFanout.Init("Fanout", 100);
  timeThread.Init("Threads", numThreads);

  timeGroupActualStrain.Init("Group actual suit/NT", 2);
  timeGroupPredStrain.Init("Group predicted suit/NT", 2);
  timeGroupDiffStrain.Init("Group diff suit/NT", 2);

  blockMax = 0;
  timeBlock = 0;
}
#endif


Scheduler::~Scheduler()
{
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

  for (unsigned t = 0; t < static_cast<unsigned>(numThreads); t++)
  {
    threadGroup[t] = -1;
    threadCurrGroup[t] = -1;
  }

  currGroup = -1;
}


void Scheduler::RegisterThreads(
  const int n)
{
  if (n == numThreads)
    return;
  numThreads = n;

  const unsigned nu = static_cast<unsigned>(n);
  threadGroup.resize(nu);
  threadCurrGroup.resize(nu);
  threadToHand.resize(nu);

#ifdef DDS_SCHEDULER
  timeThread.Init("Threads", numThreads);
  timersThread.resize(numThreads);
#endif
}


void Scheduler::RegisterRun(
  const enum RunMode mode,
  const boards& bds,
  const playTracesBin& pl)
{
  for (int b = 0; b < bds.noOfBoards; b++)
    hands[b].depth = pl.plays[b].number;
  
  Scheduler::RegisterRun(mode, bds);
}


void Scheduler::RegisterRun(
  const enum RunMode mode,
  const boards& bds)
{
  Scheduler::Reset();

  numHands = bds.noOfBoards;

  // First split the hands according to strain and hash key.
  // This will lead to a few random collisions as well.

  Scheduler::MakeGroups(bds);

  // Then check whether groups with at least two elements are
  // homogeneous or whether they need to be split.

  Scheduler::FinetuneGroups();

  Scheduler::SortHands(mode);
}


void Scheduler::SortHands(const enum RunMode mode)
{
  // Make predictions per group.

  if (mode == DDS_RUN_SOLVE)
    Scheduler::SortSolve();
  else if (mode == DDS_RUN_CALC)
    Scheduler::SortCalc();
  else if (mode == DDS_RUN_TRACE)
    Scheduler::SortTrace();
}


void Scheduler::MakeGroups(const boards& bds)
{
  deal const * dl;
  listType * lp;

  for (int b = 0; b < numHands; b++)
  {
    dl = &bds.deals[b];

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
    hands[b].fanout = Scheduler::Fanout(* dl);
    // hands[b].strength = Scheduler::Strength(* dl);

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
  const int hno1,
  const int hno2) const
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


int Scheduler::Strength(const deal& dl) const
{
  // If the strength in all suits is evenly split, then the
  // "strength" returned is close to 0. Maximum is 49.

  const unsigned sp = (dl.remainCards[0][0] | dl.remainCards[2][0]) >> 2;
  const unsigned he = (dl.remainCards[0][1] | dl.remainCards[2][1]) >> 2;
  const unsigned di = (dl.remainCards[0][2] | dl.remainCards[2][2]) >> 2;
  const unsigned cl = (dl.remainCards[0][3] | dl.remainCards[2][3]) >> 2;

  const int hsp = highCards[sp];
  const int hhe = highCards[he];
  const int hdi = highCards[di];
  const int hcl = highCards[cl];

  int dev = (hsp >= 14 ? hsp - 14 : 14 - hsp) +
    (hhe >= 14 ? hhe - 14 : 14 - hhe) +
    (hdi >= 14 ? hdi - 14 : 14 - hdi) +
    (hcl >= 14 ? hcl - 14 : 14 - hcl);

  if (dev >= 50) dev = 49;

  return dev;
}


int Scheduler::Fanout(const deal& dl) const
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
      c = static_cast<int>(dl.remainCards[h][s] >> 2);
      fanoutSuit += groupData[c].lastGroup + 1;
      if (c == 0)
        numVoids++;
    }
    fanoutSuit += numVoids * fanoutSuit;
    fanout += fanoutSuit;
  }

  return fanout;
}


schedType Scheduler::GetNumber(const int thrId)
{
  const unsigned tu = static_cast<unsigned>(thrId);
  int g = threadGroup[tu];
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

    // Atomic.
    g = ++currGroup;

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
    threadGroup[tu] = g;
    threadCurrGroup[tu] = g;
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

  threadToHand[tu] = st.number;

  if (lp->first == -1)
    threadGroup[tu] = -1;

  return st;
}


int Scheduler::NumGroups() const
{
  return numGroups;
}


#ifdef DDS_SCHEDULER
void Scheduler::StartThreadTimer(const int thrId)
{
  timersThread[thrId].Reset();
  timersThread[thrId].Start();
}


void Scheduler::EndThreadTimer(const int thrId)
{
  timersThread[thrId].End();
  int timeUser = timersThread[thrId].UserTime();

  hands[ threadToHand[thrId] ].time = timeUser;
  hands[ threadToHand[thrId] ].thread = thrId;

  group[ threadCurrGroup[thrId] ].actual += timeUser;
}


void Scheduler::StartBlockTimer()
{
  timerBlock.Reset();
  timerBlock.Start();
}


void Scheduler::EndBlockTimer()
{
  timerBlock.End();
  const int timeUserBlock = timerBlock.UserTime();

  handType * hp;
  for (int b = 0; b < numHands; b++)
  {
    hp = &hands[b];
    int timeUser = hp->time;
    double timesq = (double) timeUser * (double) timeUser;

    if (hp->selectFlag)
    {
      TimeStat ts;
      ts.Set(timeUser, timesq);

      timeStrain.Add(hp->NTflag, ts);
      timeRepeat.Add(hp->repeatNo, ts);
      timeDepth.Add(hp->depth, ts);
      timeStrength.Add(hp->strength, ts);
      timeFanout.Add(hp->fanout, ts);
      timeThread.Add(hp->thread, ts);
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

    TimeStat ts;

    ts.Set(group[g].actual);
    timeGroupActualStrain.Add(NTflag, ts);

    ts.Set(group[g].pred);
    timeGroupPredStrain.Add(NTflag, ts);

    int diff = group[g].actual - group[g].pred;
    ts.Set(diff);
    timeGroupDiffStrain.Add(NTflag, ts);
  }


  timeBlock += timeUserBlock;
  timeMax += blockMax;
  blockMax = 0;
}


void Scheduler::PrintTiming() const
{
  const string fname = string(DDS_SCHEDULER_PREFIX) + DDS_DEBUG_SUFFIX;
  ofstream fout;
  fout.open(fname);

  fout << timeStrain.List();
  fout << timeRepeat.List();
  fout << timeDepth.List();
  fout << timeStrength.List();
  fout << timeFanout.List();
  fout << timeThread.List();
  fout << timeGroupActualStrain.List();
  fout << timeGroupPredStrain.List();
  fout << timeGroupDiffStrain.List();

#if 0
  fout << setw(13) << "Hist" <<
    setw(10) << "Hist suit" <<
    setw(10) << "Hist NT" << "\n";
  for (int i = 0; i < 10000; i++)
  {
    if (timeHist[i] || timeHistSuit[i] || timeHistNT[i])
    {
      fout << setw(4) << i <<
        setw(9) << timeHist[i] <<
        setw(10) << timeHistSuit[i] <<
        setw(10) << timeHistNT[i] << "\n";
    }
  }
  fout << endl;
#endif

  if (timeBlock == 0)
    return;

  const double avg = 100. * (double) timeMax / (double) timeBlock;
  fout << "Largest hand" <<
    setw(13) << timeMax << 
    setw(13) << timeBlock <<
    setw(6) << setprecision(2) << fixed << avg << "%\n\n";

  fout.close();
}

#endif // DDS_SCHEDULER


int Scheduler::PredictedTime(
  deal& dl,
  int number) const
{
  int trump = dl.trump;
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

