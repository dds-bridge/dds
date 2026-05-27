#include "pch.h"
#include "DDS_Core.h"

namespace DDSCore {


    dds::dds()
    {
    }

    dds::~dds()
    {
    }

    auto dds::SolveBoard(ManagedSolverContext^ ctx, 
                         const ManagedDeal^ dl, 
                         int target, 
                         int solutions, 
                         int mode, 
                         ManagedFutureTricks^ futp) -> int
    {
		// 1. Set up native structures for the call
        Deal nativeDeal = ManagedDeal::ToNative(dl);
        FutureTricks nativeFt;

        // 2. Call native SolveBoard
        int result = ::SolveBoard(
            *ctx->native_,      // SolverContext&
            nativeDeal,         // const Deal&
            target,
            solutions,
            mode,
            &nativeFt           // FutureTricks*
        );

        // 3. Convert native → managed
        ManagedFutureTricks^ mf = ManagedFutureTricks::FromNative(nativeFt);

        // 4. Copy back to output parameter
        futp->Nodes = mf->Nodes;
        futp->Cards = mf->Cards;

        for (int i = 0; i < mf->Cards; i++)
        {
            futp->Suit[i] = mf->Suit[i];
            futp->Rank[i] = mf->Rank[i];
            futp->Equals[i] = mf->Equals[i];
            futp->Score[i] = mf->Score[i];
        }

        return result;
    }
}