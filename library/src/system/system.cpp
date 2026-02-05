/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>

#include "system.hpp"
#include "scheduler.hpp"

extern Scheduler scheduler;

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
    FptrType solve_chunk_common,
    FptrType calc_chunk_common,
    FptrType play_chunk_common,
    FduplType detect_solve_duplicates,
    FduplType detect_calc_duplicates,
    FduplType detect_play_duplicates,
    FsingleType solve_single_common,
    FsingleType calc_single_common,
    FsingleType play_single_common,
    FcopyType copy_solve_single,
    FcopyType copy_calc_single,
    FcopyType copy_play_single
)
{
  run_ptr_list_.resize(DDS_SYSTEM_THREAD_SIZE);
  run_ptr_list_[DDS_SYSTEM_THREAD_BASIC] = &System::run_threads_basic; 
  run_ptr_list_[DDS_SYSTEM_THREAD_WINAPI] = &System::run_threads_winapi; 
  run_ptr_list_[DDS_SYSTEM_THREAD_OPENMP] = &System::run_threads_openmp; 
  run_ptr_list_[DDS_SYSTEM_THREAD_GCD] = &System::run_threads_gcd; 
  run_ptr_list_[DDS_SYSTEM_THREAD_BOOST] = &System::run_threads_boost; 
  run_ptr_list_[DDS_SYSTEM_THREAD_STL] = &System::run_threads_stl; 
  run_ptr_list_[DDS_SYSTEM_THREAD_TBB] = &System::run_threads_tbb; 
  run_ptr_list_[DDS_SYSTEM_THREAD_STLIMPL] = 
    &System::run_threads_stlimpl; 
  run_ptr_list_[DDS_SYSTEM_THREAD_PPLIMPL] = 
    &System::run_threads_pplimpl; 

  callback_simple_list_[static_cast<size_t>(RunMode::DDS_RUN_SOLVE)] = solve_chunk_common;
  callback_simple_list_[static_cast<size_t>(RunMode::DDS_RUN_CALC)] = calc_chunk_common;
  callback_simple_list_[static_cast<size_t>(RunMode::DDS_RUN_TRACE)] = play_chunk_common;

  callback_dupl_list_[static_cast<size_t>(RunMode::DDS_RUN_SOLVE)] = detect_solve_duplicates;
  callback_dupl_list_[static_cast<size_t>(RunMode::DDS_RUN_CALC)] = detect_calc_duplicates;
  callback_dupl_list_[static_cast<size_t>(RunMode::DDS_RUN_TRACE)] = detect_play_duplicates;

  callback_single_list_[static_cast<size_t>(RunMode::DDS_RUN_SOLVE)] = solve_single_common;
  callback_single_list_[static_cast<size_t>(RunMode::DDS_RUN_CALC)] = calc_single_common;
  callback_single_list_[static_cast<size_t>(RunMode::DDS_RUN_TRACE)] = play_single_common;

  callback_copy_list_[static_cast<size_t>(RunMode::DDS_RUN_SOLVE)] = copy_solve_single;
  callback_copy_list_[static_cast<size_t>(RunMode::DDS_RUN_CALC)] = copy_calc_single;
  callback_copy_list_[static_cast<size_t>(RunMode::DDS_RUN_TRACE)] = copy_play_single;
  System::reset();
}


System::~System()
{
}


void System::reset()
{
  run_cat_ = RunMode::DDS_RUN_SOLVE;
  num_threads_ = 1;
  preferred_system_ = DDS_SYSTEM_THREAD_BASIC;

  available_system_.resize(DDS_SYSTEM_THREAD_SIZE);
  available_system_[DDS_SYSTEM_THREAD_BASIC] = true;
  for (unsigned i = 1; i < DDS_SYSTEM_THREAD_SIZE; i++)
    available_system_[i] = false;

#ifdef DDS_THREADS_WINAPI
  available_system_[DDS_SYSTEM_THREAD_WINAPI] = true;
#endif

#ifdef DDS_THREADS_OPENMP
  available_system_[DDS_SYSTEM_THREAD_OPENMP] = true;
#endif

#ifdef DDS_THREADS_GCD
  available_system_[DDS_SYSTEM_THREAD_GCD] = true;
#endif

#ifdef DDS_THREADS_BOOST
  available_system_[DDS_SYSTEM_THREAD_BOOST] = true;
#endif

#ifdef DDS_THREADS_STL
  available_system_[DDS_SYSTEM_THREAD_STL] = true;
#endif

#ifdef DDS_THREADS_TBB
  available_system_[DDS_SYSTEM_THREAD_TBB] = true;
#endif

#ifdef DDS_THREADS_STLIMPL
  available_system_[DDS_SYSTEM_THREAD_STLIMPL] = true;
#endif

#ifdef DDS_THREADS_PPLIMPL
  available_system_[DDS_SYSTEM_THREAD_PPLIMPL] = true;
#endif

  // Take the first of any multi-threading system defined.
  for (unsigned k = 1; k < available_system_.size(); k++)
  {
    if (available_system_[k])
    {
      preferred_system_ = k;
      break;
    }
  }
}


void System::get_hardware(
  int& core_count,
  unsigned long long& kilobytes_free) const
{
  kilobytes_free = 0;
  core_count = System::get_cores();

#if defined(_WIN32) || defined(__CYGWIN__)
  // Using GlobalMemoryStatusEx instead of GlobalMemoryStatus
  // was suggested by Lorne Anderson.
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  GlobalMemoryStatusEx(&statex);
  kilobytes_free = static_cast<unsigned long long>(
                    statex.ullTotalPhys / 1024);

  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  core_count = static_cast<int>(sysinfo.dwNumberOfProcessors);
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
  fscanf(fifo, "%lld", &kilobytes_free);
  fclose(fifo);

  kilobytes_free /= 1024;
  if (kilobytes_free > 500000)
  {
    kilobytes_free -= 500000;
  }

  core_count = sysconf(_SC_NPROCESSORS_ONLN);
  return;
#endif

#ifdef __linux__
  // Use half of the physical memory
  long pages = sysconf (_SC_PHYS_PAGES);
  long pagesize = sysconf (_SC_PAGESIZE);
  if (pages > 0 && pagesize > 0)
    kilobytes_free = static_cast<unsigned long long>(pages * pagesize / 1024 / 2);
  else
    kilobytes_free = 1024 * 1024; // guess 1GB

  core_count = sysconf(_SC_NPROCESSORS_ONLN);
  return;
#endif
}


int System::register_params(
  const int n_threads,
  const int mem_usable_mb)
{
  // No upper limit -- caveat emptor.
  if (n_threads < 1)
    return RETURN_THREAD_INDEX;

  num_threads_ = n_threads;
  sys_mem_mb_ = mem_usable_mb;
  return RETURN_NO_FAULT;
}


int System::register_run(
  const RunMode run_mode,
  const Boards& boards)
{
  if (run_mode >= RunMode::DDS_RUN_SIZE)
    return RETURN_THREAD_MISSING; // Not quite right;

  run_cat_ = run_mode;
  boards_ = &boards;
  return RETURN_NO_FAULT;
}


bool System::is_single_threaded() const
{
  return (preferred_system_ == DDS_SYSTEM_THREAD_BASIC);
}


bool System::is_impl() const
{
  return (preferred_system_ >= DDS_SYSTEM_THREAD_STLIMPL);
}


bool System::thread_ok(const int thread_id) const
{
  return (thread_id >= 0 && thread_id < num_threads_);
}


int System::prefer_threading(const unsigned code)
{
  if (code >= DDS_SYSTEM_THREAD_SIZE)
    return RETURN_THREAD_MISSING;

  if (! available_system_[code])
    return RETURN_THREAD_MISSING;

  preferred_system_ = code;
  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                           Basic                                  //
//////////////////////////////////////////////////////////////////////

int System::run_threads_basic()
{
  (*fptr_)(0);
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


int System::run_threads_winapi()
{
#ifdef DDS_THREADS_WINAPI
  HANDLE * solveAllEvents = static_cast<HANDLE * >(
    malloc(static_cast<unsigned>(num_threads_) * sizeof(HANDLE)));

  for (int k = 0; k < num_threads_; k++)
  {
    solveAllEvents[k] = CreateEvent(NULL, FALSE, FALSE, 0);
    if (solveAllEvents[k] == 0)
      return RETURN_THREAD_CREATE;
  }

  vector<WinWrapType> winWrap;
  const unsigned nt = static_cast<unsigned>(num_threads_);
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
    static_cast<unsigned>(num_threads_), solveAllEvents, TRUE, INFINITE);

  if (solveAllWaitResult != WAIT_OBJECT_0)
    return RETURN_THREAD_WAIT;

  for (int k = 0; k < num_threads_; k++)
    CloseHandle(solveAllEvents[k]);

  free(solveAllEvents);
#endif

  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                           OpenMP                                 //
//////////////////////////////////////////////////////////////////////

int System::run_threads_openmp()
{
#ifdef DDS_THREADS_OPENMP
  // Added after suggestion by Dirk Willecke.
  if (omp_get_dynamic())
    omp_set_dynamic(0);

  omp_set_num_threads(num_threads_);

  #pragma omp parallel default(none)
  {
    #pragma omp for schedule(dynamic)
    for (int k = 0; k < num_threads_; k++)
    {
      int thrId = omp_get_thread_num();
      (*fptr_)(thrId);
    }
  }
#endif

  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                            GCD                                   //
//////////////////////////////////////////////////////////////////////

int System::run_threads_gcd()
{
#ifdef DDS_THREADS_GCD
  dispatch_apply(static_cast<size_t>(num_threads_),
    dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0),
    ^(size_t t)
  {
    int thrId = static_cast<int>(t);
    (*fptr_)(thrId);
  });
#endif

  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                           Boost                                  //
//////////////////////////////////////////////////////////////////////

int System::run_threads_boost()
{
#ifdef DDS_THREADS_BOOST
  vector<boost::thread *> threads;

  const unsigned nu = static_cast<unsigned>(num_threads_);
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

int System::run_threads_stl()
{
#ifdef DDS_THREADS_STL
  vector<thread *> threads;

  vector<int> uniques;
  vector<int> crossrefs;
  (* callback_dupl_list_[runCat])(* boards_, uniques, crossrefs);

  const unsigned nu = static_cast<unsigned>(num_threads_);
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


int System::run_threads_stlimpl()
{
#ifdef DDS_THREADS_STLIMPL
  vector<int> uniques;
  vector<int> crossrefs;
  (* callback_dupl_list_[runCat])(* boards_, uniques, crossrefs);

  static atomic<int> thrIdNext = 0;
  bool err = false;

  ThreadMgr::instance().Reset(num_threads_);

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
      (* callback_single_list_[run_cat_])(realThrId, bno);

    if (! ThreadMgr::instance()::instance().Release(thrId))
      err = true;
  });

  if (err)
  {
    cout << "Too many threads, num_threads_ " << num_threads_ << endl;
    return RETURN_THREAD_INDEX;
  }

  (* CallbackCopyList[runCat])(crossrefs);
#endif

  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                            TBB                                   //
//////////////////////////////////////////////////////////////////////

int System::run_threads_tbb()
{
#ifdef DDS_THREADS_TBB
  vector<tbb::tbb_thread *> threads;

  const unsigned nu = static_cast<unsigned>(num_threads_);
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


int System::run_threads_pplimpl()
{
#ifdef DDS_THREADS_PPLIMPL
  vector<int> uniques;
  vector<int> crossrefs;
  (* callback_dupl_list_[runCat])(* boards_, uniques, crossrefs);

  static atomic<int> thrIdNext = 0;
  bool err = false, err2 = false;

  ThreadMgr::instance().Reset(num_threads_);

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
    cout << "Too many threads, num_threads_ " << num_threads_ << endl;
    return RETURN_THREAD_INDEX;
  }
  else if (err2)
  {
    cout << "Release failed, num_threads_ " << num_threads_ << endl;
    return RETURN_THREAD_INDEX;
  }

  (* callback_copy_list_[run_cat_])(crossrefs);
#endif

  return RETURN_NO_FAULT;
}



int System::run_threads()
{
  fptr_ = callback_simple_list_[static_cast<size_t>(run_cat_)];

  return (this->*run_ptr_list_[preferred_system_])();
}


//////////////////////////////////////////////////////////////////////
//                     Self-identification                          //
//////////////////////////////////////////////////////////////////////

string System::get_version(
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


string System::get_system(int& sys) const
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


string System::get_bits(int& bits) const
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


string System::get_compiler(int& comp) const
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


string System::get_constructor(int& cons) const
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


int System::get_cores() const
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


string System::get_threading(int& thr) const
{
  string st = "";
  thr = 0;
  for (unsigned k = 0; k < DDS_SYSTEM_THREAD_SIZE; k++)
  {
    if (available_system_[k])
    {
      st += " " + DDS_SYSTEM_THREADING[k];
      if (k == preferred_system_)
      {
        st += "(*)";
        thr = static_cast<int>(k);
      }
    }
  }
  return st;
}