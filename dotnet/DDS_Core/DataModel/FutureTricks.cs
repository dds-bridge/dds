using System;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Stores the result of a double dummy analysis for a single position.
/// 
/// Contains the number of nodes searched, the number of cards in the result,
/// and arrays for each card's suit, rank, equality group, and score.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct FutureTricks
{
    /// <summary>Number of nodes searched in the analysis.</summary>
    public int Nodes;

    /// <summary>Number of cards in the results.</summary>
    public int NumberOfCards;

    /// <summary>Suit of each card (13 entries).</summary>
    public intArray13 Suit;

    /// <summary>Rank of each card (13 entries).</summary>
    public intArray13 Ranks;

    /// <summary>Equality group for each card (13 entries).</summary>
    public intArray13 EqualGroups;

    /// <summary>Score for each card (13 entries).</summary>
    public intArray13 Score;
}
