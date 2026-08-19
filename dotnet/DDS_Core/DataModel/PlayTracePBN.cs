using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// PBN format play trace (cards played in sequence).
/// 
/// Uses PBN string representation for played cards.
/// </summary>
[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct PlayTracePBN
{
    /// <summary>Number of cards played.</summary>
    public int NumberOfPlayedCards;

    /// <summary>Cards in PBN format (max 106 characters).</summary>
     public string106 Cards;
   
}
