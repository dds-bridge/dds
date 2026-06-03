using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Multiple PBN play traces for batch analysis.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct PlayTracesPBN
{
    /// <summary>Number of boards.</summary>
    public int NumberOfBoards;

    /// <summary>Array of PBN play traces (up to MAXNOOFBOARDS).</summary>
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = DdsConstants.MaxNumberOfBoards)]
    public PlayTracePBN[] Plays;

    public PlayTracesPBN()
    {
        Plays = new PlayTracePBN[DdsConstants.MaxNumberOfBoards];
    }
}
