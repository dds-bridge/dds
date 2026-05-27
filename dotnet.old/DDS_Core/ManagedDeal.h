#pragma once
#include <dds.hpp>

namespace DDSCore {
    public ref class ManagedDeal
    {
    public:
        int Trump;
        int First;

        array<int>^ CurrentTrickSuit;
        array<int>^ CurrentTrickRank;
        array<unsigned int, 2>^ RemainCards;

        ManagedDeal();
    internal:
        static ManagedDeal^ FromNative(const ::Deal& d);
        static ::Deal ToNative(const ManagedDeal^ md);
    };
}