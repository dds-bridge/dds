using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Multiple deals for batch double dummy table calculation.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct DdTableDeals
{
    /// <summary>Number of tables.</summary>
    public int no_of_tables;

    /// <summary>Array of deals (up to MAXNOOFTABLES * DDS_STRAINS).</summary>
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = DdsConstants.MAXNOOFTABLES * DdsConstants.DDS_STRAINS)]
    public DdTableDeal[] deals;
}
