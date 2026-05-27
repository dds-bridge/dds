#pragma once
#include "pch.h"
#include "ManagedTypes.h"
#include "ManagedSolverConfig.h"

namespace DDSCore {
    inline ManagedSolverConfig::ManagedSolverConfig()
    {
        TTKind = ManagedTTKind::Large;
        DefaultMemoryMB = 0;
        MaximumMemoryMB = 0;
    }

    inline ::SolverConfig ManagedSolverConfig::ToNative(ManagedSolverConfig^ cfg)
    {
        ::SolverConfig nc;
        nc.tt_kind_ = static_cast<::TTKind>(cfg->TTKind);
        nc.tt_mem_default_mb_ = cfg->DefaultMemoryMB;
        nc.tt_mem_maximum_mb_ = cfg->MaximumMemoryMB;
        return nc;
    }

    inline ManagedSolverConfig^ ManagedSolverConfig::FromNative(const ::SolverConfig& cfg)
    {
        ManagedSolverConfig^ mc = gcnew ManagedSolverConfig();
        mc->TTKind = static_cast<ManagedTTKind>(cfg.tt_kind_);
        mc->DefaultMemoryMB = cfg.tt_mem_default_mb_;
        mc->MaximumMemoryMB = cfg.tt_mem_maximum_mb_;
        return mc;
    }
}
