/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#include <iomanip>
#include <sstream>
#include <string.h>

#include "System.h"
#include "Memory.h"
#include "Scheduler.h"

extern Scheduler scheduler;
extern Memory memory;

// Boost: Disable some header warnings.

#ifdef DDS_THREADS_BOOST
  #ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4061 4191 4619 4623 5031)
  #endif

  #include <boost/thread.hpp>

  #ifdef _MSC_VER
    #pragma warning(pop)
  #endif
#endif

#ifdef DDS_THREADS_GCD
  #include <dispatch/dispatch.h>
#endif

#ifdef DDS_THREADS_STL
  #include <thread>
#endif

#ifdef DDS_THREADS_STLIMPL
  #include <execution>
#endif

#ifdef DDS_THREADS_PPLIMPL
  #ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4355 4619 5038)
  #endif

  #include "ppl.h"

  #ifdef _MSC_VER
    #pragma warning(pop)
  #endif
#endif

#ifdef DDS_THREADS_TBB
  #ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4574)
  #endif 

  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wold-style-cast"
  #pragma GCC diagnostic ignored "-Wsign-conversion"
  #pragma GCC diagnostic ignored "-Wctor-dtor-privacy"

  #include "tbb/tbb.h"
  #include "tbb/tbb_thread.h"

  #pragma GCC diagnostic pop

  #ifdef _MSC_VER
    #pragma warning(pop)
  #endif
#endif

const vector<string> DDS_SYSTEM_PLATFORM =
{
  "",
  "Windows",
  "Cygwin",
  "Linux",
  "Apple"
};

const vector<string> DDS_SYSTEM_COMPILER =
{
  "",
  "Microsoft Visual C++",
  "MinGW",
  "GNU g++",
  "clang"
};

const vector<string> DDS_SYSTEM_CONSTRUCTOR =
{
  "",
  "DllMain",
  "Unix-style"
};

const vector<string> DDS_SYSTEM_THREADING =
{
  "None",
  "Windows",
  "OpenMP",
  "GCD",
  "Boost",
  "STL",
  "TBB",
  "STL-impl",
  "PPL-impl"
};

#define DDS_SYSTEM_THREAD_BASIC 0
#define DDS_SYSTEM_THREAD_WINAPI 1
#define DDS_SYSTEM_THREAD_OPENMP 2
#define DDS_SYSTEM_THREAD_GCD 3
#define DDS_SYSTEM_THREAD_BOOST 4
#define DDS_SYSTEM_THREAD_STL 5
#define DDS_SYSTEM_THREAD_TBB 6
#define DDS_SYSTEM_THREAD_STLIMPL 7
#define DDS_SYSTEM_THREAD_PPLIMPL 8
#define DDS_SYSTEM_THREAD_SIZE 9


System::System(
    fptrType solve_chunk_common,
    fptrType calc_chunk_common,
    fptrType play_chunk_common,
    fduplType detect_solve_duplicates,
    fduplType detect_calc_duplicates,
    fduplType detect_play_duplicates,
    fsingleType solve_single_common,
    fsingleType calc_single_common,
    fsingleType play_single_common,
    fcopyType copy_solve_single,
    fcopyType copy_calc_single,
    fcopyType copy_play_single
)
{
  RunPtrList.resize(DDS_SYSTEM_THREAD_SIZE);
  RunPtrList[DDS_SYSTEM_THREAD_BASIC] = &System::RunThreadsBasic; 
  RunPtrList[DDS_SYSTEM_THREAD_WINAPI] = &System::RunThreadsWinAPI; 
  RunPtrList[DDS_SYSTEM_THREAD_OPENMP] = &System::RunThreadsOpenMP; 
  RunPtrList[DDS_SYSTEM_THREAD_GCD] = &System::RunThreadsGCD; 
  RunPtrList[DDS_SYSTEM_THREAD_BOOST] = &System::RunThreadsBoost; 
  RunPtrList[DDS_SYSTEM_THREAD_STL] = &System::RunThreadsSTL; 
  RunPtrList[DDS_SYSTEM_THREAD_TBB] = &System::RunThreadsTBB; 
  RunPtrList[DDS_SYSTEM_THREAD_STLIMPL] = 
    &System::RunThreadsSTLIMPL; 
  RunPtrList[DDS_SYSTEM_THREAD_PPLIMPL] = 
    &System::RunThreadsPPLIMPL; 

  CallbackSimpleList[DDS_RUN_SOLVE] = solve_chunk_common;
  CallbackSimpleList[DDS_RUN_CALC] = calc_chunk_common;
  CallbackSimpleList[DDS_RUN_TRACE] = play_chunk_common;

  CallbackDuplList[DDS_RUN_SOLVE] = detect_solve_duplicates;
  CallbackDuplList[DDS_RUN_CALC] = detect_calc_duplicates;
  CallbackDuplList[DDS_RUN_TRACE] = detect_play_duplicates;

  CallbackSingleList[DDS_RUN_SOLVE] = solve_single_common;
  CallbackSingleList[DDS_RUN_CALC] = calc_single_common;
  CallbackSingleList[DDS_RUN_TRACE] = play_single_common;

  CallbackCopyList[DDS_RUN_SOLVE] = copy_solve_single;
  CallbackCopyList[DDS_RUN_CALC] = copy_calc_single;
  CallbackCopyList[DDS_RUN_TRACE] = copy_play_single;
  System::Reset();
}


System::~System()
{
}


void System::Reset()
{
  runCat = DDS_RUN_SOLVE;
  numThreads = 1;
  preferredSystem = DDS_SYSTEM_THREAD_BASIC;

  availableSystem.resize(DDS_SYSTEM_THREAD_SIZE);
  availableSystem[DDS_SYSTEM_THREAD_BASIC] = true;
  for (unsigned i = 1; i < DDS_SYSTEM_THREAD_SIZE; i++)
    availableSystem[i] = false;

#ifdef DDS_THREADS_WINAPI
  availableSystem[DDS_SYSTEM_THREAD_WINAPI] = true;
#endif

#ifdef DDS_THREADS_OPENMP
  availableSystem[DDS_SYSTEM_THREAD_OPENMP] = true;
#endif

#ifdef DDS_THREADS_GCD
  availableSystem[DDS_SYSTEM_THREAD_GCD] = true;
#endif

#ifdef DDS_THREADS_BOOST
  availableSystem[DDS_SYSTEM_THREAD_BOOST] = true;
#endif

#ifdef DDS_THREADS_STL
  availableSystem[DDS_SYSTEM_THREAD_STL] = true;
#endif

#ifdef DDS_THREADS_TBB
  availableSystem[DDS_SYSTEM_THREAD_TBB] = true;
#endif

#ifdef DDS_THREADS_STLIMPL
  availableSystem[DDS_SYSTEM_THREAD_STLIMPL] = true;
#endif

#ifdef DDS_THREADS_PPLIMPL
  availableSystem[DDS_SYSTEM_THREAD_PPLIMPL] = true;
#endif

  // Take the first of any multi-threading system defined.
  for (unsigned k = 1; k < availableSystem.size(); k++)
  {
    if (availableSystem[k])
    {
      preferredSystem = k;
      break;
    }
  }
}


void System::GetHardware(
  int& ncores,
  unsigned long long& kilobytesFree) const
{
  kilobytesFree = 0;
  ncores = System::GetCores();

#if defined(_WIN32) || defined(__CYGWIN__)
  // Using GlobalMemoryStatusEx instead of GlobalMemoryStatus
  // was suggested by Lorne Anderson.
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  GlobalMemoryStatusEx(&statex);
  kilobytesFree = static_cast<unsigned long long>(
                    statex.ullTotalPhys / 1024);

  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  ncores = static_cast<int>(sysinfo.dwNumberOfProcessors);
  return;
#endif

#ifdef __APPLE__
  // The code for Mac OS X was suggested by Matthew Kidd.

  // This is physical memory, rather than "free" memory as below 
  // for Linux.  Always leave 0.5 GB for the OS and other stuff. 
  // It would be better to find free memory (how?) but in practice 
  // the number of cores rather than free memory is almost certainly 
  // the limit for Macs which have  standardized hardware (whereas 
  // say a 32 core Linux server is hardly unusual).
  FILE * fifo = popen("sysctl -n hw.memsize", "r");
  fscanf(fifo, "%lld", &kilobytesFree);
  fclose(fifo);

  kilobytesFree /= 1024;
  if (kilobytesFree > 500000)
  {
    kilobytesFree -= 500000;
  }

  ncores = sysconf(_SC_NPROCESSORS_ONLN);
  return;
#endif

#ifdef __linux__
  // Use half of the physical memory
  long pages = sysconf (_SC_PHYS_PAGES);
  long pagesize = sysconf (_SC_PAGESIZE);
  if (pages > 0 && pagesize > 0)
    kilobytesFree = static_cast<unsigned long long>(pages * pagesize / 1024 / 2);
  else
    kilobytesFree = 1024 * 1024; // guess 1GB

  ncores = sysconf(_SC_NPROCESSORS_ONLN);
  return;
#endif
}


int System::RegisterParams(
  const int nThreads,
  const int mem_usable_MB)
{
  // No upper limit -- caveat emptor.
  if (nThreads < 1)
    return RETURN_THREAD_INDEX;

  numThreads = nThreads;
  sysMem_MB = mem_usable_MB;
  return RETURN_NO_FAULT;
}


int System::RegisterRun(
  const RunMode mode,
  const boards& bdsIn)
{
  if (mode >= DDS_RUN_SIZE)
    return RETURN_THREAD_MISSING; // Not quite right;

  runCat = mode;
  bop = &bdsIn;
  return RETURN_NO_FAULT;
}


bool System::IsSingleThreaded() const
{
  return (preferredSystem == DDS_SYSTEM_THREAD_BASIC);
}


bool System::IsIMPL() const
{
  return (preferredSystem >= DDS_SYSTEM_THREAD_STLIMPL);
}


bool System::ThreadOK(const int thrId) const
{
  return (thrId >= 0 && thrId < numThreads);
}


int System::PreferThreading(const unsigned code)
{
  if (code >= DDS_SYSTEM_THREAD_SIZE)
    return RETURN_THREAD_MISSING;

  if (! availableSystem[code])
    return RETURN_THREAD_MISSING;

  preferredSystem = code;
  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                           Basic                                  //
//////////////////////////////////////////////////////////////////////

int System::RunThreadsBasic()
{
  (*fptr)(0);
  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                           WinAPI                                 //
//////////////////////////////////////////////////////////////////////

#ifdef DDS_THREADS_WINAPI
struct WinWrapType
{
  int thrId;
  fptrType fptr;
  HANDLE *waitPtr;
};

DWORD CALLBACK WinCallback(void * p);

DWORD CALLBACK WinCallback(void * p)
{
  WinWrapType * winWrap = static_cast<WinWrapType *>(p);
  (*(winWrap->fptr))(winWrap->thrId);

  if (SetEvent(winWrap->waitPtr[winWrap->thrId]) == 0)
    return 0;

  return 1;
}
#endif


int System::RunThreadsWinAPI()
{
#ifdef DDS_THREADS_WINAPI
  HANDLE * solveAllEvents = static_cast<HANDLE * >(
    malloc(static_cast<unsigned>(numThreads) * sizeof(HANDLE)));

  for (int k = 0; k < numThreads; k++)
  {
    solveAllEvents[k] = CreateEvent(NULL, FALSE, FALSE, 0);
    if (solveAllEvents[k] == 0)
      return RETURN_THREAD_CREATE;
  }

  vector<WinWrapType> winWrap;
  const unsigned nt = static_cast<unsigned>(numThreads);
  winWrap.resize(nt);

  for (unsigned k = 0; k < nt; k++)
  {
    winWrap[k].thrId = static_cast<int>(k);
    winWrap[k].fptr = fptr;
    winWrap[k].waitPtr = solveAllEvents;

    int res = QueueUserWorkItem(WinCallback,
      static_cast<void *>(&winWrap[k]), WT_EXECUTELONGFUNCTION);
    if (res != 1)
      return res;
  }

  DWORD solveAllWaitResult;
  solveAllWaitResult = WaitForMultipleObjects(
    static_cast<unsigned>(numThreads), solveAllEvents, TRUE, INFINITE);

  if (solveAllWaitResult != WAIT_OBJECT_0)
    return RETURN_THREAD_WAIT;

  for (int k = 0; k < numThreads; k++)
    CloseHandle(solveAllEvents[k]);

  free(solveAllEvents);
#endif

  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                           OpenMP                                 //
//////////////////////////////////////////////////////////////////////

int System::RunThreadsOpenMP()
{
#ifdef DDS_THREADS_OPENMP
  // Added after suggestion by Dirk Willecke.
  if (omp_get_dynamic())
    omp_set_dynamic(0);

  omp_set_num_threads(numThreads);

  #pragma omp parallel default(none)
  {
    #pragma omp for schedule(dynamic)
    for (int k = 0; k < numThreads; k++)
    {
      int thrId = omp_get_thread_num();
      (*fptr)(thrId);
    }
  }
#endif

  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                            GCD                                   //
//////////////////////////////////////////////////////////////////////

int System::RunThreadsGCD()
{
#ifdef DDS_THREADS_GCD
  dispatch_apply(static_cast<size_t>(numThreads),
    dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0),
    ^(size_t t)
  {
    int thrId = static_cast<int>(t);
    (*fptr)(thrId);
  });
#endif

  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                           Boost                                  //
//////////////////////////////////////////////////////////////////////

int System::RunThreadsBoost()
{
#ifdef DDS_THREADS_BOOST
  vector<boost::thread *> threads;

  const unsigned nu = static_cast<unsigned>(numThreads);
  threads.resize(nu);

  for (unsigned k = 0; k < nu; k++)
    threads[k] = new boost::thread(fptr, k);

  for (unsigned k = 0; k < nu; k++)
  {
    threads[k]->join();
    delete threads[k];
  }
#endif

  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                            STL                                   //
//////////////////////////////////////////////////////////////////////

int System::RunThreadsSTL()
{
#ifdef DDS_THREADS_STL
  vector<thread *> threads;

  vector<int> uniques;
  vector<int> crossrefs;
  (* CallbackDuplList[runCat])(* bop, uniques, crossrefs);

  const unsigned nu = static_cast<unsigned>(numThreads);
  threads.resize(nu);

  for (unsigned k = 0; k < nu; k++)
    threads[k] = new thread(fptr, k);

  for (unsigned k = 0; k < nu; k++)
  {
    threads[k]->join();
    delete threads[k];
  }
#endif

  return RETURN_NO_FAULT;
}


int System::RunThreadsSTLIMPL()
{
#ifdef DDS_THREADS_STLIMPL
  vector<int> uniques;
  vector<int> crossrefs;
  (* CallbackDuplList[runCat])(* bop, uniques, crossrefs);

  static atomic<int> thrIdNext = 0;
  bool err = false;

  ThreadMgr::instance().Reset(numThreads);

  for_each(std::execution::par, uniques.begin(), uniques.end(),
    [&](int &bno)
  {
    thread_local int thrId = -1;
    thread_local int realThrId;
    if (thrId == -1)
      thrId = thrIdNext++;

    realThrId = ThreadMgr::instance()::instance().Occupy(thrId);

    if (realThrId == -1)
      err = true;
    else
      (* CallbackSingleList[runCat])(realThrId, bno);

    if (! ThreadMgr::instance()::instance().Release(thrId))
      err = true;
  });

  if (err)
  {
    cout << "Too many threads, numThreads " << numThreads << endl;
    return RETURN_THREAD_INDEX;
  }

  (* CallbackCopyList[runCat])(crossrefs);
#endif

  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                            TBB                                   //
//////////////////////////////////////////////////////////////////////

int System::RunThreadsTBB()
{
#ifdef DDS_THREADS_TBB
  vector<tbb::tbb_thread *> threads;

  const unsigned nu = static_cast<unsigned>(numThreads);
  threads.resize(nu);

  for (unsigned k = 0; k < nu; k++)
    threads[k] = new tbb::tbb_thread(fptr, k);

  for (unsigned k = 0; k < nu; k++)
  {
    threads[k]->join();
    delete threads[k];
  }
#endif

  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                            PPL                                   //
//////////////////////////////////////////////////////////////////////


int System::RunThreadsPPLIMPL()
{
#ifdef DDS_THREADS_PPLIMPL
  vector<int> uniques;
  vector<int> crossrefs;
  (* CallbackDuplList[runCat])(* bop, uniques, crossrefs);

  static atomic<int> thrIdNext = 0;
  bool err = false, err2 = false;

  ThreadMgr::instance().Reset(numThreads);

  Concurrency::parallel_for_each(uniques.begin(), uniques.end(),
    [&](int &bno)
  {
    thread_local int thrId = -1;
    thread_local int realThrId;
    if (thrId == -1)
      thrId = thrIdNext++;

    realThrId = ThreadMgr::instance().Occupy(thrId);

    if (realThrId == -1)
      err = true;
    else
      (* CallbackSingleList[runCat])(realThrId, bno);

    if (! ThreadMgr::instance().Release(thrId))
      err2 = true;
  });

  if (err)
  {
    cout << "Too many threads, numThreads " << numThreads << endl;
    return RETURN_THREAD_INDEX;
  }
  else if (err2)
  {
    cout << "Release failed, numThreads " << numThreads << endl;
    return RETURN_THREAD_INDEX;
  }

  (* CallbackCopyList[runCat])(crossrefs);
#endif

  return RETURN_NO_FAULT;
}



int System::RunThreads()
{
  fptr = CallbackSimpleList[runCat];

  return (this->*RunPtrList[preferredSystem])();
}


//////////////////////////////////////////////////////////////////////
//                     Self-identification                          //
//////////////////////////////////////////////////////////////////////

string System::GetVersion(
  int& major,
  int& minor,
  int& patch) const
{
  major = DDS_VERSION / 10000;
  minor = (DDS_VERSION - major * 10000) / 100;
  patch = DDS_VERSION % 100;

  string st = to_string(major) + "." + to_string(minor) + 
    "." + to_string(patch);
  return st;
}


string System::GetSystem(int& sys) const
{
#if defined(_WIN32)
  sys = 1;
#elif defined(__CYGWIN__)
  sys = 2;
#elif defined(__linux)
  sys = 3;
#elif defined(__APPLE__)
  sys = 4;
#else
  sys = 0;
#endif
  
  return DDS_SYSTEM_PLATFORM[static_cast<unsigned>(sys)];
}


string System::GetBits(int& bits) const
{
#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable: 4127)
#endif

  string st;
  if (sizeof(void *) == 4)
  {
    bits = 32;
    st = "32 bits";
  }
  else if (sizeof(void *) == 8)
  {
    bits = 64;
    st = "64 bits";
  }
  else
  {
    bits = 0;
    st = "unknown";
  }
#ifdef _MSC_VER
  #pragma warning(pop)
#endif
  
  return st;
}


string System::GetCompiler(int& comp) const
{
#if defined(_MSC_VER)
  comp = 1;
#elif defined(__MINGW32__)
  comp = 2;
#elif defined(__clang__)
  comp = 4; // Out-of-order on purpose
#elif defined(__GNUC__)
  comp = 3;
#else
  comp = 0;
#endif

  return DDS_SYSTEM_COMPILER[static_cast<unsigned>(comp)];
}


string System::GetConstructor(int& cons) const
{
#if defined(USES_DLLMAIN)
  cons = 1;
#elif defined(USES_CONSTRUCTOR)
  cons = 2;
#else
  cons = 0;
#endif

  return DDS_SYSTEM_CONSTRUCTOR[static_cast<unsigned>(cons)];
}


int System::GetCores() const
{
  int cores = 0;
#if defined(_WIN32) || defined(__CYGWIN__)
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  cores = static_cast<int>(sysinfo.dwNumberOfProcessors);
#elif defined(__APPLE__) || defined(__linux__)
  cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif

  // TODO Think about thread::hardware_concurrency().
  // This should be standard in C++11.

  return cores;
}


string System::GetThreading(int& thr) const
{
  string st = "";
  thr = 0;
  for (unsigned k = 0; k < DDS_SYSTEM_THREAD_SIZE; k++)
  {
    if (availableSystem[k])
    {
      st += " " + DDS_SYSTEM_THREADING[k];
      if (k == preferredSystem)
      {
        st += "(*)";
        thr = static_cast<int>(k);
      }
    }
  }
  return st;
}


string System::GetThreadSizes() const
{
  int l = 0, s = 0;
  for (unsigned i = 0; i < static_cast<unsigned>(numThreads); i++)
  {
    if (memory.ThreadSize(i) == "S")
      s++;
    else
      l++;
  }

  const string st = to_string(s) + " S, " + to_string(l) + " L";
  return st;
}
