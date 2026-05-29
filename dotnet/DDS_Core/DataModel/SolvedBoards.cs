using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Solutions for multiple boards.
/// 
/// Container for results from batch board solving operations.
/// Each entry contains the complete future tricks analysis for one board.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct SolvedBoards
{
    /// <summary>Number of solved boards.</summary>
    public int no_of_boards;

    /// <summary>Array of solutions (future tricks for each board).</summary>
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = DdsConstants.MAXNOOFBOARDS)]
    public FutureTricks[] solved_board;
}
