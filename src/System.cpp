/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#include <iomanip>
#include <sstream>

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

#include "../include/dll.h"
#include "dds.h"

#include "System.h"
#include "SolveBoard.h"
#include "PlayAnalyser.h"

extern int noOfThreads;

#define DDS_SYSTEM_THREAD_BASIC 0
#define DDS_SYSTEM_THREAD_WINAPI 1
#define DDS_SYSTEM_THREAD_OPENMP 2
#define DDS_SYSTEM_THREAD_GCD 3
#define DDS_SYSTEM_THREAD_BOOST 4
#define DDS_SYSTEM_THREAD_SIZE 5

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
  "Boost"
};


System::System()
{
  System::Reset();
}


System::~System()
{
}


void System::Reset()
{
  runCat = DDS_SYSTEM_SOLVE;
  numThreads = 1;
  preferredSystem = DDS_SYSTEM_THREAD_BASIC;

  availableSystem.resize(DDS_SYSTEM_THREAD_SIZE);

  availableSystem[DDS_SYSTEM_THREAD_BASIC] = true;

#if (defined(_MSC_VER) && !defined(DDS_THREADS_SINGLE))
  availableSystem[DDS_SYSTEM_THREAD_WINAPI] = true;
#else
  availableSystem[DDS_SYSTEM_THREAD_WINAPI] = false;
#endif

#if (defined(_OPENMP) && !defined(DDS_THREADS_SINGLE))
  availableSystem[DDS_SYSTEM_THREAD_OPENMP] = true;
#else
  availableSystem[DDS_SYSTEM_THREAD_OPENMP] = false;
#endif

#if ((defined(__IPHONE_OS_VERSION_MAX_ALLOWED) || \
      defined(__MAC_OS_X_VERSION_MAX_ALLOWED)) && \
      !defined(DDS_THREADS_SINGLE))
  availableSystem[DDS_SYSTEM_THREAD_GCD] = true;
#else
  availableSystem[DDS_SYSTEM_THREAD_GCD] = false;
#endif

#if (defined(DDS_THREADS_BOOST) && !defined(DDS_THREADS_SINGLE))
  availableSystem[DDS_SYSTEM_THREAD_BOOST] = true;
#else
  availableSystem[DDS_SYSTEM_THREAD_BOOST] = false;
#endif
  
  RunPtrList.resize(DDS_SYSTEM_THREAD_SIZE);
  RunPtrList[DDS_SYSTEM_THREAD_BASIC] = &System::RunThreadsBasic; 
  RunPtrList[DDS_SYSTEM_THREAD_WINAPI] = &System::RunThreadsWinAPI; 
  RunPtrList[DDS_SYSTEM_THREAD_OPENMP] = &System::RunThreadsOpenMP; 
  RunPtrList[DDS_SYSTEM_THREAD_GCD] = &System::RunThreadsGCD; 
  RunPtrList[DDS_SYSTEM_THREAD_BOOST] = &System::RunThreadsBoost; 

  // TODO Correct functions
  CallbackSimpleList.resize(DDS_SYSTEM_SIZE);
  CallbackSimpleList[DDS_SYSTEM_SOLVE] = SolveChunkCommon;
  CallbackSimpleList[DDS_SYSTEM_CALC] = SolveChunkCommon;
  CallbackSimpleList[DDS_SYSTEM_PLAY] = PlayChunkCommon;

  CallbackComplexList.resize(DDS_SYSTEM_SIZE);
  CallbackComplexList[DDS_SYSTEM_SOLVE] = SolveChunkDDtableCommon;
  CallbackComplexList[DDS_SYSTEM_CALC] = SolveChunkDDtableCommon;
  CallbackComplexList[DDS_SYSTEM_PLAY] = PlayChunkCommon;
}


void System::GetHardware(
  int& ncores,
  unsigned long long& kilobytesFree) const
{
  kilobytesFree = 0;
  ncores = 1;

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
  // The code for linux was suggested by Antony Lee.
  FILE * fifo = popen(
    "free -k | tail -n+3 | head -n1 | awk '{print $NF}'", "r");
  int ignore = fscanf(fifo, "%llu", &kilobytesFree);
  fclose(fifo);

  ncores = sysconf(_SC_NPROCESSORS_ONLN);
  return;
#endif
}


int System::Register(
  const unsigned code,
  const int nThreads)
{
  if (code >= DDS_SYSTEM_SIZE)
    return RETURN_THREAD_MISSING; // Not quite right;

  runCat = code;

  if (nThreads < 1 || nThreads >= MAXNOOFTHREADS)
    return RETURN_THREAD_INDEX;

  numThreads = nThreads;
  return RETURN_NO_FAULT;
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

#if (defined(_MSC_VER) && !defined(DDS_THREADS_SINGLE))
struct WinWrapType
{
  int thid;
  fptrType fptr;
  HANDLE *waitPtr;
};

DWORD CALLBACK WinCallback(void * p)
{
  WinWrapType * winWrap = static_cast<WinWrapType *>(p);
  (*(winWrap->fptr))(winWrap->thid);

  if (SetEvent(winWrap->waitPtr[winWrap->thid]) == 0)
    return 0;

  return 1;
}
#endif


int System::RunThreadsWinAPI()
{
#if (defined(_MSC_VER) && !defined(DDS_THREADS_SINGLE))
  HANDLE solveAllEvents[MAXNOOFTHREADS];

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
    winWrap[k].thid = static_cast<int>(k);
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

  for (int k = 0; k < noOfThreads; k++)
    CloseHandle(solveAllEvents[k]);
#endif

  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                           OpenMP                                 //
//////////////////////////////////////////////////////////////////////

int System::RunThreadsOpenMP()
{
#if (defined(_OPENMP) && !defined(DDS_THREADS_SINGLE))
  // Added after suggestion by Dirk Willecke.
  if (omp_get_dynamic())
    omp_set_dynamic(0);

  omp_set_num_threads(numThreads);

  #pragma omp parallel default(none)
  {
    #pragma omp for schedule(dynamic)
    for (int k = 0; k < numThreads; k++)
    {
      int thid = omp_get_thread_num();
      (*fptr)(thid);
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
#if ((defined(__IPHONE_OS_VERSION_MAX_ALLOWED) || \
      defined(__MAC_OS_X_VERSION_MAX_ALLOWED)) && \
      !defined(DDS_THREADS_SINGLE))
  dispatch_apply(static_cast<size_t>(noOfThreads),
    dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0),
    ^(size_t t)
  {
    thid = static_cast<int>(t);
    (*fptr)(thid);
  }
#endif

  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                           Boost                                  //
//////////////////////////////////////////////////////////////////////

int System::RunThreadsBoost()
{
#if (defined(DDS_THREADS_BOOST) && !defined(DDS_THREADS_SINGLE))
  vector<boost::thread *> threads;
  threads.resize(static_cast<unsigned>(numThreads));

  const unsigned nu = static_cast<unsigned>(numThreads);
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


int System::RunThreads(const int chunkSize)
{
  // TODO Add timing on the caller side, not here in System

  fptr = (chunkSize == 1 ? 
    CallbackSimpleList[runCat] : CallbackComplexList[runCat]);

  return (this->*RunPtrList[preferredSystem])();
}


string System::str(DDSInfo * info) const
{
  info->major = DDS_VERSION / 10000;
  info->minor = (DDS_VERSION - info->major * 10000) / 100;
  info->patch = DDS_VERSION % 100;

  sprintf(info->versionString, "%d.%d.%d",
    info->major, info->minor, info->patch);

  info->system = 0;
  info->compiler = 0;
  info->constructor = 0;
  info->threading = 0;
  info->noOfThreads = numThreads;

#if defined(_WIN32)
  info->system = 1;
#elif defined(__CYGWIN__)
  info->system = 2;
#elif defined(__linux)
  info->system = 3;
#elif defined(__APPLE__)
  info->system = 4;
#endif

  stringstream ss;
  ss << "DDS DLL\n-------\n";
  ss << left << setw(13) << "System" <<
    setw(20) << right << 
      DDS_SYSTEM_PLATFORM[static_cast<unsigned>(info->system)] << "\n";

#if defined(_MSC_VER)
  info->compiler = 1;
#elif defined(__MINGW32__)
  info->compiler = 2;
#elif defined(__GNUC__)
  info->compiler = 3;
#elif defined(__clang__)
  info->compiler = 4;
#endif
  ss << left << setw(13) << "Compiler" <<
    setw(20) << right << 
      DDS_SYSTEM_COMPILER[static_cast<unsigned>(info->compiler)] << "\n";

#if defined(USES_DLLMAIN)
  info->constructor = 1;
#elif defined(USES_CONSTRUCTOR)
  info->constructor = 2;
#endif
  ss << left << setw(13) << "Constructor" <<
    setw(20) << right << 
      DDS_SYSTEM_CONSTRUCTOR[static_cast<unsigned>(info->constructor)] << 
      "\n";

  ss << left << setw(9) << "Threading";
  string sy = "";
  for (unsigned k = 0; k < DDS_SYSTEM_THREAD_SIZE; k++)
  {
    if (availableSystem[k])
    {
      sy += " " + DDS_SYSTEM_THREADING[k];
      if (k == preferredSystem)
        sy += "(*)";
    }
  }
  ss << setw(24) << right << sy << "\n";

  ss << left << setw(17) << "Number of threads" <<
    setw(16) << right << noOfThreads << "\n";

  strcpy(info->systemString, ss.str().c_str());
  return ss.str();
}

