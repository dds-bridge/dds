/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_INIT_H
#define DDS_INIT_H

#include <api/dds.h>
#include "system/Memory.h"
#include <solver_context/SolverContext.h>
#include <memory>


void SetDeal(const std::shared_ptr<ThreadData>& thrp);

void SetDealTables(SolverContext& ctx);

void InitWinners(
  const deal& dl,
  pos& posPoint,
  const std::shared_ptr<ThreadData>& thrp);

void CloseDebugFiles();

#endif
