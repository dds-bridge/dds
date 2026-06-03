using System;
using System.Runtime.InteropServices;

namespace DDS_Core;


/// <summary>
/// Analyzed result of a play sequence.
/// 
/// Contains tricks won for each possible play continuation.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct SolvedPlay
{
    /// <summary>Number of results.</summary>
    public int NumberOfResults;

    /// <summary>Tricks possible after each play (53 entries).</summary>
    public IntArray53 Tricks;
}
