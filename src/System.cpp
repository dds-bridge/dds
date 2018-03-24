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

#ifdef DDS_THREADS_GCD
  #include <dispatch/dispatch.h>
#endif

#ifdef DDS_THREADS_STL
  #include <thread>
#endif

#include "../include/dll.h"
#include "dds.h"

#include "System.h"
#include "SolveBoard.h"
#include "PlayAnalyser.h"

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
  "STL"
};

#define DDS_SYSTEM_THREAD_BASIC 0
#define DDS_SYSTEM_THREAD_WINAPI 1
#define DDS_SYSTEM_THREAD_OPENMP 2
#define DDS_SYSTEM_THREAD_GCD 3
#define DDS_SYSTEM_THREAD_BOOST 4
#define DDS_SYSTEM_THREAD_STL 5
#define DDS_SYSTEM_THREAD_SIZE 6


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

#ifdef DDS_THREADS_WINAPI
  availableSystem[DDS_SYSTEM_THREAD_WINAPI] = true;
#else
  availableSystem[DDS_SYSTEM_THREAD_WINAPI] = false;
#endif

#ifdef DDS_THREADS_OPENMP
  availableSystem[DDS_SYSTEM_THREAD_OPENMP] = true;
#else
  availableSystem[DDS_SYSTEM_THREAD_OPENMP] = false;
#endif

#ifdef DDS_THREADS_GCD
  availableSystem[DDS_SYSTEM_THREAD_GCD] = true;
#else
  availableSystem[DDS_SYSTEM_THREAD_GCD] = false;
#endif

#ifdef DDS_THREADS_BOOST
  availableSystem[DDS_SYSTEM_THREAD_BOOST] = true;
#else
  availableSystem[DDS_SYSTEM_THREAD_BOOST] = false;
#endif

#ifdef DDS_THREADS_STL
  availableSystem[DDS_SYSTEM_THREAD_STL] = true;
#else
  availableSystem[DDS_SYSTEM_THREAD_STL] = false;
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
  
  RunPtrList.resize(DDS_SYSTEM_THREAD_SIZE);
  RunPtrList[DDS_SYSTEM_THREAD_BASIC] = &System::RunThreadsBasic; 
  RunPtrList[DDS_SYSTEM_THREAD_WINAPI] = &System::RunThreadsWinAPI; 
  RunPtrList[DDS_SYSTEM_THREAD_OPENMP] = &System::RunThreadsOpenMP; 
  RunPtrList[DDS_SYSTEM_THREAD_GCD] = &System::RunThreadsGCD; 
  RunPtrList[DDS_SYSTEM_THREAD_BOOST] = &System::RunThreadsBoost; 
  RunPtrList[DDS_SYSTEM_THREAD_STL] = &System::RunThreadsSTL; 

  // DDS_SYSTEM_CALC_ doesn't happen.
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
  (void) System::GetCores(ncores);

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


int System::RegisterParams(
  const int nThreads,
  const int mem_usable_MB,
  const int mem_def_MB,
  const int mem_max_MB)
{
  if (nThreads < 1 || nThreads >= MAXNOOFTHREADS)
    return RETURN_THREAD_INDEX;

  numThreads = nThreads;
  sysMem_MB = mem_usable_MB;
  thrDef_MB = mem_def_MB;
  thrMax_MB = mem_max_MB;
  return RETURN_NO_FAULT;
}


int System::RegisterRun(const unsigned code)
{
  // TODO Use same code as in Scheduler, put in dds.h

  if (code >= DDS_SYSTEM_SIZE)
    return RETURN_THREAD_MISSING; // Not quite right;

  runCat = code;
  return RETURN_NO_FAULT;
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
  int thid;
  fptrType fptr;
  HANDLE *waitPtr;
};

DWORD CALLBACK WinCallback(void * p);

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
#ifdef DDS_THREADS_WINAPI
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

  for (int k = 0; k < numThreads; k++)
    CloseHandle(solveAllEvents[k]);
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
#ifdef DDS_THREADS_GCD
  dispatch_apply(static_cast<size_t>(numThreads),
    dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0),
    ^(size_t t)
  {
    int thid = static_cast<int>(t);
    (*fptr)(thid);
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


//////////////////////////////////////////////////////////////////////
//                            STL                                   //
//////////////////////////////////////////////////////////////////////

int System::RunThreadsSTL()
{
#ifdef DDS_THREADS_STL
  vector<thread *> threads;
  threads.resize(static_cast<unsigned>(numThreads));

  const unsigned nu = static_cast<unsigned>(numThreads);
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


int System::RunThreads(const int chunkSize)
{
  fptr = (chunkSize == 1 ? 
    CallbackSimpleList[runCat] : CallbackComplexList[runCat]);

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
#ellse
  sys = 0;
#endif
  
  return DDS_SYSTEM_PLATFORM[static_cast<unsigned>(sys)];
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


string System::GetCores(int& cores) const
{
#if defined(_WIN32) || defined(__CYGWIN__)
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  cores = static_cast<int>(sysinfo.dwNumberOfProcessors);
#elif defined(__APPLE__) || defined(__linux__)
  cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif

  // TODO Think about thread::hardware_concurrency().
  // This should be standard in C++11.

  return to_string(cores);
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


string System::str(DDSInfo * info) const
{
  stringstream ss;
  ss << "DDS DLL\n-------\n";

  const string strSystem = System::GetSystem(info->system);
  ss << left << setw(13) << "System" <<
    setw(20) << right << strSystem << "\n";

  const string strCompiler = System::GetCompiler(info->compiler);
  ss << left << setw(13) << "Compiler" <<
    setw(20) << right << strCompiler << "\n";

  const string strConstructor = System::GetConstructor(info->constructor);
  ss << left << setw(13) << "Constructor" <<
    setw(20) << right << strConstructor << "\n";

  const string strVersion = System::GetVersion(info->major,
    info->minor, info->patch);
  ss << left << setw(13) << "Version" <<
    setw(20) << right << strVersion << "\n";
  strcpy(info->versionString, strVersion.c_str());

  ss << left << setw(17) << "Memory max (MB)" <<
    setw(16) << right << sysMem_MB << "\n";

  const string stm = to_string(thrDef_MB) + " to " + to_string(thrMax_MB);
  ss << left << setw(17) << "Thread def (MB)" <<
    setw(16) << right << stm << "\n";

  System::GetCores(info->numCores);
  ss << left << setw(17) << "Number of cores" <<
    setw(16) << right << info->numCores << "\n";

  info->noOfThreads = numThreads;
  ss << left << setw(17) << "Number of threads" <<
    setw(16) << right << numThreads << "\n";

  const string strThreading = System::GetThreading(info->threading);
  ss << left << setw(9) << "Threading" <<
    setw(24) << right << strThreading << "\n";

  const string st = ss.str();
  strcpy(info->systemString, st.c_str());
  return st;
}

