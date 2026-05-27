#pragma once
#include "ManagedTypes.h"

namespace DDSCore {
    public ref class ManagedSolverConfig
    {
    public:
        ManagedTTKind TTKind;
        int DefaultMemoryMB;
        int MaximumMemoryMB;

        ManagedSolverConfig();

    internal:
        static ::SolverConfig ToNative(ManagedSolverConfig^ cfg);

        static ManagedSolverConfig^ FromNative(const ::SolverConfig& cfg);
    };
}