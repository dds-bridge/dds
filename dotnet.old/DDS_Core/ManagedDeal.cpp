#include "pch.h"
#include "ManagedDeal.h"

namespace DDSCore {
    ManagedDeal::ManagedDeal()
    {
        CurrentTrickSuit = gcnew array<int>(3);
        CurrentTrickRank = gcnew array<int>(3);
        RemainCards = gcnew array<unsigned int, 2>(DDS_HANDS, DDS_SUITS);
    }

    ManagedDeal^ ManagedDeal::FromNative(const ::Deal& d)
    {
        ManagedDeal^ md = gcnew ManagedDeal();

        md->Trump = d.trump;
        md->First = d.first;

        for (int i = 0; i < 3; i++)
        {
            md->CurrentTrickSuit[i] = d.currentTrickSuit[i];
            md->CurrentTrickRank[i] = d.currentTrickRank[i];
        }

        for (int h = 0; h < DDS_HANDS; h++)
            for (int s = 0; s < DDS_SUITS; s++)
                md->RemainCards[h, s] = d.remainCards[h][s];

        return md;
    }

    ::Deal ManagedDeal::ToNative(const ManagedDeal^ md)
    {
        ::Deal d;

        d.trump = md->Trump;
        d.first = md->First;

        for (int i = 0; i < 3; i++)
        {
            d.currentTrickSuit[i] = md->CurrentTrickSuit[i];
            d.currentTrickRank[i] = md->CurrentTrickRank[i];
        }

        for (int h = 0; h < DDS_HANDS; h++)
            for (int s = 0; s < DDS_SUITS; s++)
                d.remainCards[h][s] = md->RemainCards[h, s];

        return d;
    }
}