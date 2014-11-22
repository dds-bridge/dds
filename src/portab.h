/* 
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund / 
   2014 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


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
  #define USES_CONSTRUCTOR
      /* DLL uses a constructor function for initialization */
      
  typedef long long __int64;

#elif defined (__APPLE__)
  #include <unistd.h>
  #define USES_CONSTRUCTOR
  typedef long long __int64;

#endif

#if (! defined DDS_THREADS_WIN32)  && \
    (! defined DDS_THREADS_OPENMP) && \
    (! defined DDS_THREADS_NONE)
#    define DDS_THREADS_NONE
#endif

#ifdef _OPENMP
 #include <omp.h>
#endif


// In C++11 UNUSED(x) is explicitly provided
#if __cplusplus <= 199711L
  #if defined (_MSC_VER)
    #define UNUSED(x) (void) (x);
  #else
    #define UNUSED(x) (void) (sizeof((x), 0))
  #endif
  #ifndef __clang__
     #define nullptr NULL
  #endif
#endif
