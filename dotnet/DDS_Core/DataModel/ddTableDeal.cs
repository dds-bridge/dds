using System;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// A single deal for double dummy table calculation.
/// 
/// Contains card distribution as bitmasks for efficient computation.
/// C++ type: unsigned int cards[DDS_HANDS][DDS_SUITS] = unsigned int cards[4][4]
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct DdTableDeal
{
    /// <summary>
    /// Cards for each hand and suit using 2D indexing.
    /// Access: cards[hand, suit] where hand=0-3 (N/E/S/W), suit=0-3 (S/H/D/C)
    /// Each uint is a bitmask of cards (bit 0 = Deuce, bit 12 = Ace)
    /// </summary>
    public FourHands Cards;
}
