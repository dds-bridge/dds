#if defined(_WIN32)
  #if defined(__MINGW32__) && !defined(WINVER)
      #define WINVER 0x500 
  #endif
  #include <windows.h>
  #include <process.h>
  
  #define USES_DLLMAIN
      /* DLL uses DllMain() for initialization */

  #define MULTITHREADING_WINDOWS
      /* This only means that function calls are used to determine
         the number of cores etc.  It does not in itself mean that
	 the DLL is multi-threaded, see DDS_THREADS_*.  */


      /* SH: stdafx.h is not really required.  It is used to
         precompile Windows header files, but the CL compiler
	 is plenty fast for this particular project. */
  #if defined (_MSC_VER)
    #include <intrin.h>
    /* #include stdafx.h */
  #endif

#elif defined (__CYGWIN__)
  #include <windows.h>
  #include <process.h>
  #define USES_DLLMAIN
  #define MULTITHREADING_WINDOWS

#elif defined (__linux)
  #include <unistd.h>
  #define USES_CONSTRUCTOR
      /* DLL uses a constructor function for initialization */
      
  #define MULTITHREADING_OPENMP
      /* As above, this just means that OpenMP functions are
         used to find the number of cores. */

  typedef long long __int64;

#elif defined (__APPLE__)
  #include <unistd.h>
  #define USES_CONSTRUCTOR
  #define MULTITHREADING_OPENMP
  typedef long long __int64;

#endif

#if (! defined DDS_THREADS_WIN32)  && \
    (! defined DDS_THREADS_OPENMP) && \
    (! defined DDS_THREADS_NONE)
#    define DDS_THREADS_NONE
#endif

#ifdef DDS_THREADS_OPENMP
  #include <omp.h>
#endif

/*#include "stdafx.h"*/
