using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Represents a bridge Deal in PBN (Portable Bridge Notation) format.
/// 
/// PBN format is a standard text representation for bridge hands.
/// Example: "N:AKQ.K.AKQ.AKQ8 J976.QJT.J42.Q2 T842.A542.T83.T3 53.9876.965.J976"
/// </summary>
[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct DealPBN
{
    /// <summary>Trump suit (0 = NT, 1 = Spades, 2 = Hearts, 3 = Diamonds, 4 = Clubs).</summary>
    public int trump;

    /// <summary>Hand to play first (0 = N, 1 = E, 2 = S, 3 = W).</summary>
    public int first;

    /// <summary>Suits of cards played in the current trick (3 entries).</summary>
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
    public int[] currentTrickSuit;

    /// <summary>Ranks of cards played in the current trick (3 entries).</summary>
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
    public int[] currentTrickRank;

    /// <summary>PBN string describing remaining cards (max 80 characters).</summary>
       public ByteBuffer80 remainCards;
}
