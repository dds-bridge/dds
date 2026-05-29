using System;
using System.Runtime.InteropServices;

namespace DDS_Core;


/// <summary>
/// Multiple binary play traces for batch analysis.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct PlayTracesBin
{
    /// <summary>Number of boards.</summary>
    public int no_of_boards;

    /// <summary>Array of play traces (up to MAXNOOFBOARDS).</summary>
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = DdsConstants.MAXNOOFBOARDS)]
    public PlayTraceBin[] plays;
}
