/**
 * @file portab.h
 * @brief Portability macros and definitions for DDS (Double Dummy Solver).
 *
 * This header defines macros and typedefs to ensure DDS can be built and run on multiple platforms,
 * including Windows, Linux, and macOS. It handles compiler, OS, and threading differences.
 *
 * @copyright (C) 2006-2014 by Bo Haglund / 2014-2018 by Bo Haglund & Soren Hein.
 */

#ifndef DDS_PORTAB_H
#define DDS_PORTAB_H


#if defined(_WIN32)
  #if defined(__MINGW32__) && !defined(WINVER)
    #define WINVER 0x500
  #endif

  #include <windows.h>
  #include <process.h>

  #define USES_DLLMAIN
  /* DLL uses DllMain() for initialization */

  #if defined (_MSC_VER)
    #include <intrin.h>
  #endif

#elif defined (__CYGWIN__)
  #include <windows.h>
  #include <process.h>

  #define USES_DLLMAIN

#elif defined (__linux)
  #include <unistd.h>
  #if !defined(DDS_NO_STATIC_INIT)
  #define USES_CONSTRUCTOR
  /* DLL uses a constructor function for initialization */
  #endif

  typedef long long __int64;

#elif defined (__APPLE__)
  #include <unistd.h>

  #define USES_CONSTRUCTOR

  typedef long long __int64;

#endif

#if (! defined DDS_THREADS_WIN32) && \
    (! defined DDS_THREADS_OPENMP) && \
    (! defined DDS_THREADS_NONE)
  #define DDS_THREADS_NONE
#endif

#endif
