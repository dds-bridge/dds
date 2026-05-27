using System;
using System.Linq;
using System.Runtime.InteropServices;

namespace DDS_Core.DataModel;

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct FutureTricks
{
    public long Nodes;

    public long TotalNodes;
    public long TotalTrickNodes;
    public long LeafNodes;
    public long TTLookups;
    public long TTHits;
    public long TTInserts;
    public long TTUpdates;
    public long QuickTricks1;
    public long QuickTricks2;
    public long LaterTricks;
    public long CheckSum;

    public int cards;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 13)]
    public int[] suit;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 13)]
    public int[] rank;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 13)]
    public int[] equals;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 13)]
    public int[] score;
}
