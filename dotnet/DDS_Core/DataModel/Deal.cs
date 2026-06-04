using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using DDS_Core.Helpers;

namespace DDS_Core;

/// <summary>
/// Represents a bridge Deal for double dummy analysis.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct Deal
{
    /// <summary>Trump suit (0 = NT, 1 = Spades, 2 = Hearts, 3 = Diamonds, 4 = Clubs).</summary>
    public int Trump;

    /// <summary>Hand to play first (0 = N, 1 = E, 2 = S, 3 = W).</summary>
    public int First;

    /// <summary>Suits of cards played in the current trick (3 entries).</summary>
    public intArray3 CurrentTrickSuit;

    /// <summary>Ranks of cards played in the current trick (3 entries).</summary>
    public intArray3 CurrentTrickRank;

    /// <summary>Remaining cards for each hand and suit (4 hands × 4 suits = 16 elements).</summary>
    public FourHands RemainingCards;
}
