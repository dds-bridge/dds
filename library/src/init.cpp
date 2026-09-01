/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <algorithm>
#include <cstring>
#include <cstdio>
#include <iomanip>
#include <ios>
#include <memory>
#include <sstream>

#include <calc_tables.hpp>
#include "init.hpp"
#include <play_analyser.hpp>
#include <solve_board.hpp>
#include <lookup_tables/lookup_tables.hpp>
#include <solver_context/solver_context.hpp>
#include <string>
#include <system/scheduler.hpp>
#include <system/system.hpp>
#include <system/thread_mgr.hpp>
#include <trans_table/trans_table.hpp>
#include <utility/constants.h>
#include <api/dll.h>
#include <utility/debug.h>

System sysdep;
Memory memory;
Scheduler scheduler;

void InitDebugFiles();


int _initialized = 0;


/*
 * Initialize the solver's static memory: TT memory pools, scheduler /
 * thread-manager state, and one-time lookup-table setup.
 *
 * Public API documentation is maintained in the API headers.
 */
void STDCALL InitializeStaticMemory()
{
  SetResources(0, 0);
}


/*
 * Deprecated alias for InitializeStaticMemory(). The thread count is no
 * longer meaningful (internal batch threading was removed), so the argument
 * is ignored.
 *
 * Public API documentation is maintained in the API headers.
 */
void STDCALL SetMaxThreads(
  int userThreads)
{
  (void) userThreads;
  InitializeStaticMemory();
}


/*
 * Set memory and thread resources for the solver.
 *
 * Public API documentation is maintained in the API headers.
 */
void STDCALL SetResources(
  int maxMemoryMB,
  int maxThreadsIn)
{
  // Figure out system resources.
  int ncores;
  unsigned long long kilobytesFree;
  sysdep.get_hardware(ncores, kilobytesFree);

  // Memory usage will be limited to the lower of:
  // - maxMemoryMB + 30% (if given; statistically this works out)
  // - 70% of free memory
  // - 1800 MB if we're on a 32-bit system.

  const int memMaxGivenMB = (maxMemoryMB == 0 ? 1000000 : 
    static_cast<int>(1.3 * maxMemoryMB));
  const int memMaxFreeMB = static_cast<int>(0.70 * kilobytesFree / 1024);
  const int memMax32bMB = (sizeof(void *) == 4 ? 1800 : 1000000);

  int memMaxMB = min(memMaxGivenMB, memMaxFreeMB);
  memMaxMB = min(memMaxMB, memMax32bMB);

  // Internal parallel execution has been removed.
  // Legacy API calls execute sequentially, so use a single internal thread.
  if (maxThreadsIn > 1) {
    std::fprintf(
      stderr,
      "DDS warning: SetResources maxThreadsIn=%d requested, but internal batch threading is disabled; using 1 thread.\n",
      maxThreadsIn);
  }
  (void) maxThreadsIn;
  (void) ncores;
  const int thrMax = 1;

  // For simplicity we won't vary the amount of memory per thread
  // in the small and large versions.

  int noOfThreads, noOfLargeThreads, noOfSmallThreads;
  if (thrMax * THREADMEM_LARGE_MAX_MB <= memMaxMB)
  {
    // We have enough memory for the maximum number of large threads.
    noOfThreads = thrMax;
    noOfLargeThreads = thrMax;
    noOfSmallThreads = 0;
  }
  else if (thrMax * THREADMEM_SMALL_MAX_MB > memMaxMB)
  {
    // We don't even have enough memory for only small threads.
    // We'll limit the number of threads.
    noOfThreads = static_cast<int>(memMaxMB / 
      static_cast<double>(THREADMEM_SMALL_MAX_MB));
    noOfLargeThreads = 0;
    noOfSmallThreads = noOfThreads;
  }
  else
  {
    // We'll have a mixture with as many large threads as possible.
    const double d = static_cast<double>(
          THREADMEM_LARGE_MAX_MB - THREADMEM_SMALL_MAX_MB);

    noOfThreads = thrMax;
    noOfLargeThreads = static_cast<int>(
      (memMaxMB - thrMax * THREADMEM_SMALL_MAX_MB) / d);
    noOfSmallThreads = thrMax - noOfLargeThreads;
  }

  sysdep.register_params(noOfThreads, memMaxMB);

  scheduler.RegisterThreads(noOfThreads);

  // Clear the thread memory and fill it up again.
  memory.Resize(0, DDS_TT_SMALL, 0, 0);
  if (noOfLargeThreads > 0)
    memory.Resize(static_cast<unsigned>(noOfLargeThreads),
      DDS_TT_LARGE, THREADMEM_LARGE_DEF_MB, THREADMEM_LARGE_MAX_MB);
  if (noOfSmallThreads > 0)
    memory.Resize(static_cast<unsigned>(noOfThreads),
      DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);

  ThreadMgr::instance().Reset(noOfThreads);

  InitDebugFiles();

  if (! _initialized)
  {
    _initialized = 1;
    init_lookup_tables();
  }
}


/*
 * Set the threading backend used by the solver.
 *
 * Public API documentation is maintained in the API headers.
 */
int STDCALL SetThreading(
  int code)
{
  return sysdep.prefer_threading(static_cast<unsigned>(code));
}


void InitDebugFiles()
{
}


void SetDeal(
  const std::shared_ptr<ThreadData>& thrp)
{
  /* Initialization of the rel structure is inspired by
     a solution given by Thomas Andrews */

  for (int s = 0; s < DDS_SUITS; s++)
  {
    thrp->lookAheadPos.aggr[s] = 0;
    for (int h = 0; h < DDS_HANDS; h++)
    {
      thrp->lookAheadPos.rank_in_suit[h][s] = thrp->suit[h][s];
      thrp->lookAheadPos.aggr[s] |= thrp->suit[h][s];
    }
  }

  for (int s = 0; s < DDS_SUITS; s++)
  {
    for (int h = 0; h < DDS_HANDS; h++)
      thrp->lookAheadPos.length[h][s] = static_cast<unsigned char>(
  count_table[thrp->lookAheadPos.rank_in_suit[h][s]]);
  }

  // Clubs are implicit, for a given trick number.
  for (int h = 0; h < DDS_HANDS; h++)
  {
    thrp->lookAheadPos.hand_dist[h] =
      static_cast<long long>(
        (thrp->lookAheadPos.length[h][0] << 8) |
        (thrp->lookAheadPos.length[h][1] << 4) |
        (thrp->lookAheadPos.length[h][2] ));
  }
}


void SetDealTables(
  SolverContext& ctx)
{
  auto thrp = ctx.thread();
  unsigned int topBitRank = 1;
  unsigned int topBitNo = 2;

  // Initialization of the rel structure is inspired by
  // a solution given by Thomas Andrews.

  // rel[aggr].abs_rank[absolute rank][suit].hand is the hand
  // (N = 0, E = 1 etc.) which holds the absolute rank in
  // the suit characterized by aggr.
  // rel[aggr].abs_rank[absolute rank][suit].rank is the
  // relative rank of that card.

  for (int s = 0; s < DDS_SUITS; s++)
  {
    for (int ord = 1; ord <= 13; ord++)
    {
      thrp->rel[0].abs_rank[ord][s].hand = -1;
      thrp->rel[0].abs_rank[ord][s].rank = 0;
    }
  }

  // handLookup[suit][absolute rank] is the hand (N = 0 etc.)
  // holding the absolute rank in suit.

  int handLookup[DDS_SUITS][15];
  for (int s = 0; s < DDS_SUITS; s++)
  {
    for (int r = 14; r >= 2; r--)
    {
      handLookup[s][r] = 0;
      for (int h = 0; h < DDS_HANDS; h++)
      {
        if (thrp->suit[h][s] & bit_map_rank[r])
        {
          handLookup[s][r] = h;
          break;
        }
      }
    }
  }

  {
    ctx.trans_table()->init(handLookup);
  }

  RelRanksType * relp;
  for (unsigned int aggr = 1; aggr < 8192; aggr++)
  {
    if (aggr >= (topBitRank << 1))
    {
      /* Next top bit */
      topBitRank <<= 1;
      topBitNo++;
    }

    thrp->rel[aggr] = thrp->rel[aggr ^ topBitRank];
    relp = &thrp->rel[aggr];

    int weight = count_table[aggr];
    for (int c = weight; c >= 2; c--)
    {
      for (int s = 0; s < DDS_SUITS; s++)
      {
        relp->abs_rank[c][s].hand = relp->abs_rank[c - 1][s].hand;
        relp->abs_rank[c][s].rank = relp->abs_rank[c - 1][s].rank;
      }
    }
    for (int s = 0; s < DDS_SUITS; s++)
    {
      relp->abs_rank[1][s].hand =
        static_cast<signed char>(handLookup[s][topBitNo]);
      relp->abs_rank[1][s].rank = static_cast<char>(topBitNo);
    }
  }
}


void InitWinners(
  const Deal& dl,
  Pos& posPoint,
  const std::shared_ptr<ThreadData>& thrp)
{
  int hand, suit, rank;
  unsigned short int startMovesBitMap[DDS_HANDS][DDS_SUITS];

  for (int h = 0; h < DDS_HANDS; h++)
    for (int s = 0; s < DDS_SUITS; s++)
      startMovesBitMap[h][s] = 0;

  for (int k = 0; k < posPoint.hand_rel_first; k++)
  {
    hand = HAND_ID(dl.first, k);
    suit = dl.currentTrickSuit[k];
    rank = dl.currentTrickRank[k];
    startMovesBitMap[hand][suit] |= bit_map_rank[rank];
  }

  int aggr;
  for (int s = 0; s < DDS_SUITS; s++)
  {
    aggr = 0;
    for (int h = 0; h < DDS_HANDS; h++)
      aggr |= startMovesBitMap[h][s] | thrp->suit[h][s];

    posPoint.winner[s].rank = thrp->rel[aggr].abs_rank[1][s].rank;
    posPoint.winner[s].hand = thrp->rel[aggr].abs_rank[1][s].hand;
    posPoint.second_best[s].rank = thrp->rel[aggr].abs_rank[2][s].rank;
    posPoint.second_best[s].hand = thrp->rel[aggr].abs_rank[2][s].hand;
  }
}


void STDCALL GetDDSInfo(DDSInfo * info)
{
  stringstream ss;
  ss << "DDS DLL\n-------\n";

  const string strSystem = sysdep.get_system(info->system);
  ss << left << setw(13) << "System" <<
    setw(20) << right << strSystem << "\n";

  const string strBits = sysdep.get_bits(info->numBits);
  ss << left << setw(13) << "Word size" <<
    setw(20) << right << strBits << "\n";

  const string strCompiler =sysdep.get_compiler(info->compiler);
  ss << left << setw(13) << "Compiler" <<
    setw(20) << right << strCompiler << "\n";

  const string strConstructor = sysdep.get_constructor(info->constructor);
  ss << left << setw(13) << "Constructor" <<
    setw(20) << right << strConstructor << "\n";

  const string strVersion = sysdep.get_version(info->major,
    info->minor, info->patch);
  ss << left << setw(13) << "Version" <<
    setw(20) << right << strVersion << "\n";
  strcpy(info->version_string, strVersion.c_str());

  ss << left << setw(17) << "Memory max (MB)" <<
    setw(16) << right << sysdep.get_memory_max() << "\n";

  const string stm = to_string(THREADMEM_SMALL_DEF_MB) + "-" + 
    to_string(THREADMEM_SMALL_MAX_MB) + " / " +
    to_string(THREADMEM_LARGE_DEF_MB) + "-" +
    to_string(THREADMEM_LARGE_MAX_MB);
  ss << left << setw(17) << "Threads (MB)" <<
    setw(16) << right << stm << "\n";

  info->numCores  = sysdep.get_cores();
  ss << left << setw(17) << "Number of cores" <<
    setw(16) << right << info->numCores << "\n";

  info->noOfThreads = sysdep.get_num_threads();
  ss << left << setw(17) << "Number of threads" <<
    setw(16) << right << sysdep.get_num_threads() << "\n";

  int l = 0, s = 0;
  for (unsigned i = 0; i < static_cast<unsigned>(info->noOfThreads); i++)
  {
    if (memory.ThreadSize(i) == "S")
      s++;
    else
      l++;
  }

  const string strThrSizes =  to_string(s) + " S, " + to_string(l) + " L";
  strcpy(info->threadSizes, strThrSizes.c_str());
  ss << left << setw(13) << "Thread sizes" <<
    setw(20) << right << strThrSizes << "\n";

  const string strThreading =  sysdep.get_threading(info->threading);
  ss << left << setw(9) << "Threading" <<
    setw(24) << right << strThreading << "\n";

  const string st = ss.str();
  strcpy(info->systemString, st.c_str());
}


/**
 * @brief Free memory used by the solver.
 */
void STDCALL FreeMemory()
{
  for (unsigned thrId = 0; thrId < memory.NumThreads(); thrId++)
    memory.ReturnThread(thrId);
}

void STDCALL ErrorMessage(int code, char line[80])
{
  switch (code)
  {
    case RETURN_NO_FAULT:
      strcpy(line, TEXT_NO_FAULT);
      break;
    case RETURN_UNKNOWN_FAULT:
      strcpy(line, TEXT_UNKNOWN_FAULT);
      break;
    case RETURN_ZERO_CARDS:
      strcpy(line, TEXT_ZERO_CARDS);
      break;
    case RETURN_TARGET_TOO_HIGH:
      strcpy(line, TEXT_TARGET_TOO_HIGH);
      break;
    case RETURN_DUPLICATE_CARDS:
      strcpy(line, TEXT_DUPLICATE_CARDS);
      break;
    case RETURN_TARGET_WRONG_LO:
      strcpy(line, TEXT_TARGET_WRONG_LO);
      break;
    case RETURN_TARGET_WRONG_HI:
      strcpy(line, TEXT_TARGET_WRONG_HI);
      break;
    case RETURN_SOLNS_WRONG_LO:
      strcpy(line, TEXT_SOLNS_WRONG_LO);
      break;
    case RETURN_SOLNS_WRONG_HI:
      strcpy(line, TEXT_SOLNS_WRONG_HI);
      break;
    case RETURN_TOO_MANY_CARDS:
      strcpy(line, TEXT_TOO_MANY_CARDS);
      break;
    case RETURN_SUIT_OR_RANK:
      strcpy(line, TEXT_SUIT_OR_RANK);
      break;
    case RETURN_PLAYED_CARD:
      strcpy(line, TEXT_PLAYED_CARD);
      break;
    case RETURN_CARD_COUNT:
      strcpy(line, TEXT_CARD_COUNT);
      break;
    case RETURN_THREAD_INDEX:
      strcpy(line, TEXT_THREAD_INDEX);
      break;
    case RETURN_MODE_WRONG_LO:
      strcpy(line, TEXT_MODE_WRONG_LO);
      break;
    case RETURN_MODE_WRONG_HI:
      strcpy(line, TEXT_MODE_WRONG_HI);
      break;
    case RETURN_TRUMP_WRONG:
      strcpy(line, TEXT_TRUMP_WRONG);
      break;
    case RETURN_FIRST_WRONG:
      strcpy(line, TEXT_FIRST_WRONG);
      break;
    case RETURN_PLAY_FAULT:
      strcpy(line, TEXT_PLAY_FAULT);
      break;
    case RETURN_PBN_FAULT:
      strcpy(line, TEXT_PBN_FAULT);
      break;
    case RETURN_TOO_MANY_BOARDS:
      strcpy(line, TEXT_TOO_MANY_BOARDS);
      break;
    case RETURN_THREAD_CREATE:
      strcpy(line, TEXT_THREAD_CREATE);
      break;
    case RETURN_THREAD_WAIT:
      strcpy(line, TEXT_THREAD_WAIT);
      break;
    case RETURN_THREAD_MISSING:
      strcpy(line, TEXT_THREAD_MISSING);
      break;
    case RETURN_NO_SUIT:
      strcpy(line, TEXT_NO_SUIT);
      break;
    case RETURN_TOO_MANY_TABLES:
      strcpy(line, TEXT_TOO_MANY_TABLES);
      break;
    case RETURN_CHUNK_SIZE:
      strcpy(line, TEXT_CHUNK_SIZE);
      break;
    case RETURN_PAR_TABLE_FAULT:
      strcpy(line, TEXT_PAR_TABLE_FAULT);
      break;
    default:
      strcpy(line, "Not a DDS error code");
      break;
  }
}

