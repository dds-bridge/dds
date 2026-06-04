using System;
using System.Drawing;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Par result for a specific dealer.
/// 
/// Contains number of par contracts and their details.
/// </summary>
[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct ParResultsDealer
{
    /// <summary>Number of contracts yielding the par score.</summary>
    public int NumberOfContracts;

    /// <summary>Par score for the specified dealer hand.</summary>
    public int Score;

    /// <summary>Par contract text strings (10 entries, max 10 chars each).</summary>
    public stringArray10x10 Contracts;
}
