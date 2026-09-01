/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

// System headers
#if defined(DDS_MEMORY_LEAKS) && defined(_MSC_VER)
  #define DDS_MEMORY_LEAKS_WIN32
  #define _CRTDBG_MAP_ALLOC
  #include <crtdbg.h>
#endif

// Aggregator for the solver's compile-time constants and data model. The
// public function-declaration surface (SolveBoard, CalcDDtable, ...) lives in
// <api/dll.h> and is included only by API consumers and the API implementation
// files, never by internal solver code.
#include <api/dds_constants.hpp>
#include <api/dds_data_types.hpp>  // also pulls in <utility/constants.h>
