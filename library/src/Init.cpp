/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "Init.h"
#include <system/System.h>
#include <system/Scheduler.h>
#include <system/ThreadMgr.h>
#include <utility/debug.h>
#include <utility/Constants.h>
#include <utility/LookupTables.h>
#include "SolveBoard.h"
#include "CalcTables.h"
#include "PlayAnalyser.h"
// Order matters: include TransTable to ensure complete type for virtual calls
#include <trans_table/TransTable.h>
#include <system/SolverContext.h>

System sysdep(
    &SolveChunkCommon,
    &CalcChunkCommon,
    &PlayChunkCommon,
    &DetectSolveDuplicates,
    &DetectCalcDuplicates,
    &DetectPlayDuplicates,
    &SolveSingleCommon,
    &CalcSingleCommon,
    &PlaySingleCommon,
    &CopySolveSingle,
    &CopyCalcSingle,
    &CopyPlaySingle
);
Memory memory;
Scheduler scheduler;

void InitDebugFiles();


int _initialized = 0;


/**
 * @brief Set the maximum number of threads used by the solver.
 *
 * @param userThreads Maximum number of threads to use
 */
void STDCALL SetMaxThreads(
  int userThreads)
{
  SetResources(0, userThreads);
}


/**
 * @brief Set memory and thread resources for the solver.
 *
 * @param maxMemoryMB Maximum memory in megabytes
 * @param maxThreadsIn Maximum number of threads
 */
void STDCALL SetResources(
  int maxMemoryMB,
  int maxThreadsIn)
{
  // Figure out system resources.
  int ncores;
  unsigned long long kilobytesFree;
  sysdep.GetHardware(ncores, kilobytesFree);

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

  // The number of threads will be limited by:
  // - If threading set as single-threaded or compiled only 
  //   single-threaded: 1
  // - If threading set as one of the IMPL variants: ncores
  //   whatever the user says (as we currently don't have control)
  // - Otherwise the lower of maxThreads and ncores

  int thrMax;
  if (sysdep.IsSingleThreaded())
    thrMax = 1;
  else if (sysdep.IsIMPL() || maxThreadsIn <= 0)
    thrMax = ncores;
  else
    thrMax = min(maxThreadsIn, ncores);

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

  sysdep.RegisterParams(noOfThreads, memMaxMB);

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


/**
 * @brief Set the threading backend used by the solver.
 *
 * @param code Threading backend code (see documentation)
 * @return 1 on success, error code otherwise
 */
int STDCALL SetThreading(
  int code)
{
  return sysdep.PreferThreading(static_cast<unsigned>(code));
}


void InitDebugFiles()
{
  for (unsigned thrId = 0; thrId < memory.NumThreads(); thrId++)
  {
    // Create a temporary context to access ThreadData for initialization.
  SolverContext tmp_ctx;
  [[maybe_unused]] auto thrp = tmp_ctx.thread();
  [[maybe_unused]] const string send = to_string(thrId) + DDS_DEBUG_SUFFIX;
  thrp->init_debug_files(send);
  }

#ifdef DDS_SCHEDULER
  InitFileScheduler();
#endif
}


void CloseDebugFiles()
{
  for (unsigned thrId = 0; thrId < memory.NumThreads(); thrId++)
  {
  SolverContext tmp_ctx;
  [[maybe_unused]] auto thrp = tmp_ctx.thread();
  thrp->close_debug_files();
  }
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
      thrp->lookAheadPos.rankInSuit[h][s] = thrp->suit[h][s];
      thrp->lookAheadPos.aggr[s] |= thrp->suit[h][s];
    }
  }

  for (int s = 0; s < DDS_SUITS; s++)
  {
    for (int h = 0; h < DDS_HANDS; h++)
      thrp->lookAheadPos.length[h][s] = static_cast<unsigned char>(
  count_table[thrp->lookAheadPos.rankInSuit[h][s]]);
  }

  // Clubs are implicit, for a given trick number.
  for (int h = 0; h < DDS_HANDS; h++)
  {
    thrp->lookAheadPos.handDist[h] =
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
      handLookup[s][r] = 0;
      for (int h = 0; h < DDS_HANDS; h++)
      {
        if (thrp->suit[h][s] & bitMapRank[r])
        {
          handLookup[s][r] = h;
          break;
        }
      }
    }
  }

  {
    ctx.transTable()->init(handLookup);
  }

  relRanksType * relp;
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
        relp->absRank[c][s].hand = relp->absRank[c - 1][s].hand;
        relp->absRank[c][s].rank = relp->absRank[c - 1][s].rank;
      }
    }
    for (int s = 0; s < DDS_SUITS; s++)
    {
      relp->absRank[1][s].hand =
        static_cast<signed char>(handLookup[s][topBitNo]);
      relp->absRank[1][s].rank = static_cast<char>(topBitNo);
    }
  }
}


void InitWinners(
  const deal& dl,
  pos& posPoint,
  const std::shared_ptr<ThreadData>& thrp)
{
  int hand, suit, rank;
  unsigned short int startMovesBitMap[DDS_HANDS][DDS_SUITS];

  for (int h = 0; h < DDS_HANDS; h++)
    for (int s = 0; s < DDS_SUITS; s++)
      startMovesBitMap[h][s] = 0;

  for (int k = 0; k < posPoint.handRelFirst; k++)
  {
    hand = handId(dl.first, k);
    suit = dl.currentTrickSuit[k];
    rank = dl.currentTrickRank[k];
    startMovesBitMap[hand][suit] |= bitMapRank[rank];
  }

  int aggr;
  for (int s = 0; s < DDS_SUITS; s++)
  {
    aggr = 0;
    for (int h = 0; h < DDS_HANDS; h++)
      aggr |= startMovesBitMap[h][s] | thrp->suit[h][s];

    posPoint.winner[s].rank = thrp->rel[aggr].absRank[1][s].rank;
    posPoint.winner[s].hand = thrp->rel[aggr].absRank[1][s].hand;
    posPoint.secondBest[s].rank = thrp->rel[aggr].absRank[2][s].rank;
    posPoint.secondBest[s].hand = thrp->rel[aggr].absRank[2][s].hand;
  }
}


void STDCALL GetDDSInfo(DDSInfo * info)
{
  stringstream ss;
  ss << "DDS DLL\n-------\n";

  const string strSystem = sysdep.GetSystem(info->system);
  ss << left << setw(13) << "System" <<
    setw(20) << right << strSystem << "\n";

  const string strBits = sysdep.GetBits(info->numBits);
  ss << left << setw(13) << "Word size" <<
    setw(20) << right << strBits << "\n";

  const string strCompiler =sysdep.GetCompiler(info->compiler);
  ss << left << setw(13) << "Compiler" <<
    setw(20) << right << strCompiler << "\n";

  const string strConstructor = sysdep.GetConstructor(info->constructor);
  ss << left << setw(13) << "Constructor" <<
    setw(20) << right << strConstructor << "\n";

  const string strVersion = sysdep.GetVersion(info->major,
    info->minor, info->patch);
  ss << left << setw(13) << "Version" <<
    setw(20) << right << strVersion << "\n";
  strcpy(info->versionString, strVersion.c_str());

  ss << left << setw(17) << "Memory max (MB)" <<
    setw(16) << right << sysdep.GetMemoryMax() << "\n";

  const string stm = to_string(THREADMEM_SMALL_DEF_MB) + "-" + 
    to_string(THREADMEM_SMALL_MAX_MB) + " / " +
    to_string(THREADMEM_LARGE_DEF_MB) + "-" +
    to_string(THREADMEM_LARGE_MAX_MB);
  ss << left << setw(17) << "Threads (MB)" <<
    setw(16) << right << stm << "\n";

  info->numCores  = sysdep.GetCores();
  ss << left << setw(17) << "Number of cores" <<
    setw(16) << right << info->numCores << "\n";

  info->noOfThreads = sysdep.GetNumThreads();
  ss << left << setw(17) << "Number of threads" <<
    setw(16) << right << sysdep.GetNumThreads() << "\n";

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

  const string strThreading =  sysdep.GetThreading(info->threading);
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
    default:
      strcpy(line, "Not a DDS error code");
      break;
  }
}

