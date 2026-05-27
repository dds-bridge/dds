using System.Runtime.InteropServices;
using DDS_Core.DataModel;

namespace DDS_Core.DataModel;

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct Boards
{
    public int noOfBoards;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 200)]
    public Deal[] deals;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 200)]
    public int[] target;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 200)]
    public int[] solutions;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 200)]
    public int[] mode;
}
