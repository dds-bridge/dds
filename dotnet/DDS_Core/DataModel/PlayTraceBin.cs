using System;
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
    public int number;

    /// <summary>Suit of each played card (52 entries).</summary>
    public IntBuffer52 suit;

    /// <summary>Rank of each played card (52 entries).</summary>
    public IntBuffer52 rank;
}
