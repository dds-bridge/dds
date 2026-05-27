using System.Runtime.InteropServices;

using DDS_Core.DataModel;

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct SolvedBoards
{
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 200)]
    public FutureTricks[] solvedBoard;
}
