/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <cstring>

#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
  #include <unistd.h>
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#endif

#include "system.hpp"

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


System::System()
{
  System::reset();
}


System::~System()
{
}


void System::reset()
{
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