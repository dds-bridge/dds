using System;
using System.Linq;
using System.Runtime.InteropServices;

namespace DDS_Core;

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct FutureTricks
{
    public int Nodes;
    public int  cards;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 13)]
    public int[] suit;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 13)]
    public int[] rank;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 13)]
    public int[] equals;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 13)]
    public int[] score;
}
