using System;
using System.Runtime.InteropServices;

namespace DDS_Core;

[StructLayout(LayoutKind.Sequential)]
public struct ParResultsBuffer
{
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = DdsConstants.MAXNOOFTABLES)]
    public ParResults[] Items;

    public void Init()
    {
        Items = new ParResults[DdsConstants.MAXNOOFTABLES];
    }
}

