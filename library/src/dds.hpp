#ifndef _DDS_EXTERNAL_CPP_API_H
#define _DDS_EXTERNAL_CPP_API_H

#if defined(__has_include)
#if __has_include(<dds/dds.h>)
#include <dds/dds.h>
#elif __has_include(<dds.h>)
#include <dds.h>
#else
#error "Unable to find DDS public header: expected <dds/dds.h> or <dds.h>"
#endif
#else
#include <dds/dds.h>
#endif

#include <api/calc_dd_table.hpp>
#include <api/calc_par.hpp>
#include <api/solve_board.hpp>

#endif // _DDS_EXTERNAL_CPP_API_H