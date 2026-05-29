using System;
using System.Runtime.InteropServices;

namespace DDS_Core;

//TODO: Tjek denne

/// <summary>
/// 2D view of the four hands (ie cards) as [hand, suit] indexing.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct Hands
{
    /// <summary>
    /// Cards for each hand and suit (4 hands × 4 suits = 16 elements).
    /// Stored as flattened array but accessed via 2D indexer.
    /// </summary>
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
    private uint[] cards;

    /// <summary>Gets or sets card by [hand, suit] indexing.</summary>
    /// <param name="hand">Hand index (0=N, 1=E, 2=S, 3=W)</param>
    /// <param name="suit">Suit index (0=Spades, 1=Hearts, 2=Diamonds, 3=Clubs)</param>
    public ref uint this[int hand, int suit]
    {
        get
        {
            if (hand < 0 || hand >= 4)
                throw new ArgumentOutOfRangeException(nameof(hand), "Hand must be 0-3");

            if (suit < 0 || suit >= 4)
                throw new ArgumentOutOfRangeException(nameof(suit), "Suit must be 0-3");

            return ref cards[hand * 4 + suit];
        }
    }
}
