using System;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Par results for all declarer/strain combinations (up to 40).
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct AllParResults
{
    /// <summary>Array of par results (up to MAXNOOFTABLES entries).</summary>
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = DdsConstants.MAXNOOFTABLES)]
    public ParResults[] par_results;
}
