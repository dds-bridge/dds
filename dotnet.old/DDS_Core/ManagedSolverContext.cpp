#include "pch.h"
#include "ManagedSolverContext.h"

namespace DDSCore {
    inline DDSCore::ManagedSolverContext::ManagedSolverContext(ManagedSolverConfig^ cfg)
    {
        native_ = new SolverContext(ManagedSolverConfig::ToNative(cfg));
    }

    inline DDSCore::ManagedSolverContext::~ManagedSolverContext()
    {
        this->!ManagedSolverContext();
    }

    inline DDSCore::ManagedSolverContext::!ManagedSolverContext()
    {
        delete native_;
        native_ = nullptr;
    }

    inline array<System::String^>^ DDSCore::ManagedSolverContext::GetLog()
    {
        auto& buf = native_->utilities().log_buffer();
        int count = static_cast<int>(buf.size());
        auto arr = gcnew array<System::String^>(count);

        for (int i = 0; i < count; i++)
            arr[i] = gcnew System::String(buf[i].c_str());
       
        return arr;
    }

    inline void DDSCore::ManagedSolverContext::ClearLog()
    {
        native_->utilities().log_clear();
    }
}