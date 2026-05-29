using System;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Analyzed results of multiple play sequences.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct SolvedPlays
{
    /// <summary>Number of solved plays.</summary>
    public int no_of_boards;

    /// <summary>Array of solved play results (up to MAXNOOFBOARDS).</summary>
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = DdsConstants.MAXNOOFBOARDS)]
    public SolvedPlay[] solved;
}
