using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Multiple boards in PBN format for batch solving.
/// 
/// Similar to Boards but uses PBN (Portable Bridge Notation) format
/// for deal representation. Used for solving multiple boards efficiently.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct BoardsPBN
{
    /// <summary>Number of boards to solve.</summary>
    public int no_of_boards;

    /// <summary>Array of deals in PBN format.</summary>
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = DdsConstants.MAXNOOFBOARDS)]
    public DealPBN[] deals;

    /// <summary>Target tricks for each board.</summary>
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = DdsConstants.MAXNOOFBOARDS)]
    public int[] target;

    /// <summary>Solution mode for each board.</summary>
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = DdsConstants.MAXNOOFBOARDS)]
    public int[] solutions;

    /// <summary>Solve mode for each board.</summary>
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = DdsConstants.MAXNOOFBOARDS)]
    public int[] mode;
}
