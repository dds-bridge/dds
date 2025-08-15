/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_PARALLEL_H
#define DDS_PARALLEL_H

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

#endif

