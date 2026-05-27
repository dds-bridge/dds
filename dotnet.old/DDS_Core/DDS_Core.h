#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "ManagedTypes.h"
#include "ManagedSolverContext.h"
#include "ManagedDeal.h"

using namespace System;

namespace DDSCore {
	public ref class dds
	{

	public:
		dds();
		~dds();

		auto SolveBoard(
			ManagedSolverContext^ ctx,
			const ManagedDeal^ dl,
			int target,
			int solutions,
			int mode,
			ManagedFutureTricks^ futp) -> int;
	};




}
