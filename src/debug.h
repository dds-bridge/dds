/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_DEBUG_H
#define DDS_DEBUG_H


/*
    A number of debug flags cause output files to be generated.

    One file per thread is generated.

    Some example statistics on what to expect for a single
    hand, single-threaded, with about 207,000 AB nodes and
    about 63,000 trick nodes, called in solve mode such that
    a total of 11 calls to the AB optimization are performed.

    The clock "ticks" are for that debugging mode only, with
    all the other ones turned off, unless otherwise noted.
    The data is of course approximate.  (Of course nothing
    gets faster with debugging.)

    Mode                Clock "ticks"   File size in KB
    No debugging                   66                --

    DDS_TOP_LEVEL                  63                 8

    DDS_AB_STATS                   71                36
    + DDS_AB_DETAILS               75                64

    DDS_AB_HITS                   680              8624 (stored)
                                                  12004 (retrieved)

    DDS_TT_STATS (*)               68                 8

    DDS_TIMING                    125                 8
    + DDS_TIMING_DETAILS          130                28

    All together                  720                --

    (*) This depends on the stat functions called in TransTable.
    Here only the summary functions are called by default.

    For comparison, the old DDS functions, which only work
    single-threaded, yields three files stat.txt, storett.txt
    and rectt.txt.

                               145124                56 (stat)
                                                   6940 (store)
                                                   7732 (rectt)

    So the speed-up is a factor of 200.
*/

// If you want ALL the output files, this is easier
// #define DDS_DEBUG_ALL

#define DDS_DEBUG_SUFFIX ".txt"

// Enables data about each call to the top-level AB routine.
// #define DDS_TOP_LEVEL
#define DDS_TOP_LEVEL_PREFIX "toplevel"

// Enables AB statistics, node counts etc.
// #define DDS_AB_STATS
#define DDS_AB_STATS_PREFIX "ABstats"

// Gives more detail, in combination with DDS_AB_STATS.
// #define DDS_AB_DETAILS

// Gives information on nodes stored and retrieved from TT memory.
// #define DDS_AB_HITS
#define DDS_AB_HITS_RETRIEVED_PREFIX "retrieved"
#define DDS_AB_HITS_STORED_PREFIX "stored"

// Gives statistics on the usage of TT memory.
// #define DDS_TT_STATS
#define DDS_TT_STATS_PREFIX "TTstats"

// Enables timing of the AB search and related functions.
// Makes an attempt to calculate exclusive times of functions.
// #define DDS_TIMING
#define DDS_TIMING_PREFIX "timer"

// Enables statistics on move generation quality.
// #define DDS_MOVES
#define DDS_MOVES_PREFIX "movestats"

// Enables timing in the scheduler.
// #define DDS_SCHEDULER
#define DDS_SCHEDULER_PREFIX "sched"


#ifdef DDS_DEBUG_ALL
  #define DDS_TOP_LEVEL
  #ifndef DDS_AB_STATS
    #define DDS_AB_STATS
  #endif
  #ifndef DDS_AB_DETAILS
    #define DDS_AB_DETAILS
  #endif
  #ifndef DDS_AB_HITS
    #define DDS_AB_HITS
  #endif
  #ifndef DDS_TT_STATS
    #define DDS_TT_STATS
  #endif
  #ifndef DDS_TIMING
    #define DDS_TIMING
  #endif
  #ifndef DDS_TIMING_DETAILS
    #define DDS_TIMING_DETAILS
  #endif
  #ifndef DDS_MOVES
    #define DDS_MOVES
  #endif
  #ifndef DDS_MOVES_DETAILS
    #define DDS_MOVES_DETAILS
  #endif
#endif


// This debugging feature only works with Microsoft's compiler.
// In fact, it doesn't seem to do anything right now :-).
// #define DDS_MEMORY_LEAKS


#define COUNTER_SLOTS 200

extern long long counter[COUNTER_SLOTS];

#endif
