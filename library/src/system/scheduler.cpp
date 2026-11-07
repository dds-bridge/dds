/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <cmath>
#include <iostream>
#include <limits>

#include "deal_fanout.hpp"
#include "scheduler.hpp"
#include <fstream>
#include <iomanip>
#include <utility>
#include <vector>
#ifdef DDS_SCHEDULER
#include <system/time_stat_list.hpp>
#include <utility/debug.h>
#endif


Scheduler::Scheduler()
{
    numThreads = 0;
    numHands = 0;

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


#ifdef DDS_SCHEDULER
void Scheduler::InitTimes()
{
    // Initialize TimeStatList members
    timeStrain.Reset();
    timeRepeat.Reset();
    timeDepth.Reset();
    timeStrength.Reset();
    timeFanout.Reset();
    timeThread.Reset();

    timeStrain.Init("Suit/NT", 2);
    timeRepeat.Init("Repeat number", 16);
    timeDepth.Init("Trace depth", 60);
    timeStrength.Init("Evenness", 60);
    timeFanout.Init("Fanout", 100);
    timeThread.Init("Threads", numThreads);

    timeGroupActualStrain.Reset();
    timeGroupPredStrain.Reset();
    timeGroupDiffStrain.Reset();

    timeGroupActualStrain.Init("Group actual suit/NT", 2);
    timeGroupPredStrain.Init("Group predicted suit/NT", 2);
    timeGroupDiffStrain.Init("Group diff suit/NT", 2);

    blockMax = 0;
    timeBlock = 0;
}
#endif


Scheduler::~Scheduler()
{ }

void Scheduler::ClearTiming()
{
#ifdef DDS_SCHEDULER
    timeStrain.Clear();
    timeRepeat.Clear();
    timeDepth.Clear();
    timeStrength.Clear();
    timeFanout.Clear();
    timeThread.Clear();
    timeGroupActualStrain.Clear();
    timeGroupPredStrain.Clear();
    timeGroupDiffStrain.Clear();
#else
    // Nothing to do if scheduler timing not compiled in; ensure hands[] times are untouched.
#endif
}


void Scheduler::Reset()
{
    for (int b = 0; b < MAXNOOFBOARDS; b++)
    {
        hands[b].next = -1;
        hands[b].repeatNo = 0;
        hands[b].depth = 0;
        hands[b].strength = 0;
        hands[b].fanout = 0;
        hands[b].thread = 0;
        hands[b].selectFlag = 0;
        hands[b].time = 0;
    }

    for (int g = 0; g < MAXNOOFBOARDS; g++)
    {
        group[g].head = -1;
        group[g].actual = 0;
        group[g].repeatNo = 0;
        group[g].pred = 0;
    }

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
    const Boards& bds,
    const PlayTracesBin& pl)
{
    for (int b = 0; b < bds.no_of_boards; b++)
        hands[b].depth = pl.plays[b].number;

    Scheduler::RegisterRun(mode, bds);
}


void Scheduler::RegisterRun(
    const enum RunMode mode,
    const Boards& bds)
{
    Scheduler::Reset();

    numHands = bds.no_of_boards;

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

    if (mode == RunMode::DDS_RUN_SOLVE)
        Scheduler::SortSolve();
    else if (mode == RunMode::DDS_RUN_CALC)
        Scheduler::SortCalc();
    else if (mode == RunMode::DDS_RUN_TRACE)
        Scheduler::SortTrace();
}


void Scheduler::MakeGroups(const Boards& bds)
{
    Deal const * dl;
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
        hands[b].fanout = dds::internal::deal_fanout(*dl);

        lp = &list[strain][key];

        if (lp->first == -1)
        {
            lp->first = b;
            lp->last = b;
            lp->length = 1;

            group[numGroups].strain = strain;
            group[numGroups].hash = key;
            group[numGroups].head = -1;
            group[numGroups].actual = 0;
            group[numGroups].repeatNo = 0;
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
            group[numGroups].head = -1;
            group[numGroups].actual = 0;
            group[numGroups].repeatNo = 0;

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
                    group[numGroups].head = -1;
                    group[numGroups].actual = 0;
                    group[numGroups].repeatNo = 0;

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

static int SORT_SOLVE_TIMES[2][8] =
{
    { 284000,  91000, 37000, 23000, 17000, 15000, 13000, 4000 },
    { 388000, 140000, 60000, 40000, 30000, 23000, 18000, 6000 },
};


// Lower end of linear, upper end of linear, slope of linear,
// exponential start, coefficient.

static double SORT_SOLVE_FANOUT[2][5] =
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

static double SORT_CALC_FANOUT[2][5] =
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

static int SORT_TRACE_TIMES[2][8] =
{
    { 157000, 47000, 26000, 18000, 16000, 14000, 10000,  6000 },
    { 205000, 87000, 45000, 36000, 32000, 28000, 24000, 20000 },
};

// Initial value for 0 and 1 cards
// Value up to 15 cards incl
// Slope between 16 and 48 incl
// Average for 49-52

static double SORT_TRACE_DEPTH[2][4] =
{
    { 0.742, 0.411, 0.0414, 1.820 },
    { 0.669, 0.428, 0.0346, 1.606 }
};

// Lower end of linear, upper end of linear, slope of linear,
// exponential start, coefficient.

static double SORT_TRACE_FANOUT[2][5] =
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

        if (hp->repeatNo == 0 && timeUser > 0)
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
        const int head = group[g].head;
        if (head < 0 || head >= numHands)
            continue;

        const int NTflag = (hands[head].strain == 4 ? 1 : 0);

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
    using std::fixed;
    using std::ofstream;
    using std::setprecision;
    using std::setw;
    using std::string;

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

void Scheduler::GetBoardTimes(std::vector<std::pair<int,int>>& outVec) const
{
    outVec.clear();
    for (int b = 0; b < numHands; b++)
    {
        const handType& hp = hands[b];
        outVec.emplace_back(b, hp.time);
    }
}


void Scheduler::SetBoardTime(int boardIndex, long long time_us)
{
    if (boardIndex < 0 || boardIndex >= MAXNOOFBOARDS) return;
    // store in the hand time field; this is a lightweight fallback
    // for when DDS_SCHEDULER isn't enabled. No locking required for
    // single-writer per-board usage pattern from the solver threads.
    hands[boardIndex].time = saturate_board_time_us(time_us);
}

auto saturate_board_time_us(long long time_us) -> int
{
    if (time_us <= 0)
        return 0;
    constexpr auto kMax = static_cast<long long>(std::numeric_limits<int>::max());
    if (time_us >= kMax)
        return std::numeric_limits<int>::max();
    return static_cast<int>(time_us);
}


