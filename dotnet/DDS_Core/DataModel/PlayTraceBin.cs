using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Binary play trace (cards played in sequence).
/// 
/// Contains card suits and ranks for cards played during the hand.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct PlayTraceBin
{
    /// <summary>Number of cards played (1-52).</summary>
    public int NumberOfCards;

    /// <summary>Suit of each card played (52 plays max).</summary>
    public intArray52 Suits;

    /// <summary>Rank of each card played (52 plays max).</summary>
    public intArray52 Ranks;

}
