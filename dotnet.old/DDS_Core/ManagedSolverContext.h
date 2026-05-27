#pragma once
//using namespace System;

#include "ManagedSolverConfig.h"

namespace DDSCore {
    public ref class ManagedSolverContext
    {
    public:
        ManagedSolverContext(ManagedSolverConfig^ cfg);

        ~ManagedSolverContext();

        !ManagedSolverContext();
        

       array<System::String^>^ GetLog();

        void ClearLog();


    internal:
        SolverContext* native_;
    };
}