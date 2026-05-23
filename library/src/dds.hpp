#ifndef _DDS_EXTERNAL_CPP_API_H
#define _DDS_EXTERNAL_CPP_API_H

#ifdef _VCXPROJ
	#include <dds.h>
#else
	#include <dds/dds.h>
#endif 

#include <api/calc_dd_table.hpp>
#include <api/calc_par.hpp>
#include <api/solve_board.hpp>

#endif // _DDS_EXTERNAL_CPP_API_H