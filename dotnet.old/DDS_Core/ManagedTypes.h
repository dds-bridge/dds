#pragma once
#include <dds.hpp>

namespace DDSCore {
    public enum class ManagedTTKind
    {
        Small = (int)::TTKind::Small,
        Large = (int)::TTKind::Large
    };

    public ref class ManagedFutureTricks
    {
    public:
        int Nodes;
        int Cards;

        array<int>^ Suit;    // length = Cards
        array<int>^ Rank;    // length = Cards
        array<int>^ Equals;  // length = Cards
        array<int>^ Score;   // length = Cards

        ManagedFutureTricks();

    internal:
        static ManagedFutureTricks^ FromNative(const FutureTricks& ft);

    };

}