using System;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Contract details (level, strain, tricks, seats).
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct ContractType
{
    /// <summary>Under tricks (0 = make, 1-13 = sacrifice).</summary>
    public int UnderTricks;

    /// <summary>Over tricks (0-3, e.g., 1 for 4S + 1).</summary>
    public int OverTricks;

    /// <summary>Contract level (1-7).</summary>
    public int Level;

    /// <summary>Denomination (0 = NT, 1 = Spades, 2 = Hearts, 3 = Diamonds, 4 = Clubs).</summary>
    public int Denomination;

    /// <summary>Seats playing contract (0 = N, 1 = E, 2 = S, 3 = W, 4 = NS, 5 = EW).</summary>
    public int Seats;
}
