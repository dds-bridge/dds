#include "pch.h"
#include "ManagedTypes.h"

namespace DDSCore {
    inline ManagedFutureTricks::ManagedFutureTricks()
    {
        // DDS returnerer maks 13 entries
        Suit = gcnew array<int>(13);
        Rank = gcnew array<int>(13);
        Equals = gcnew array<int>(13);
        Score = gcnew array<int>(13);
    }

    inline ManagedFutureTricks^ ManagedFutureTricks::FromNative(const ::FutureTricks& ft)
    {
        ManagedFutureTricks^ m = gcnew ManagedFutureTricks();

        m->Nodes = ft.nodes;
        m->Cards = ft.cards;

        for (int i = 0; i < ft.cards; i++)
        {
            m->Suit[i] = ft.suit[i];
            m->Rank[i] = ft.rank[i];
            m->Equals[i] = ft.equals[i];
            m->Score[i] = ft.score[i];
        }

        return m;
    }
}
