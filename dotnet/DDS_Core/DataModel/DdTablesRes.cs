using System;
using System.Runtime.InteropServices;

namespace DDS_Core;

//TODO: Tjek denne

/// <summary>
/// Multiple double dummy table results.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct DdTablesRes
{
    /// <summary>Number of boards.</summary>
    public int no_of_boards;

    /// <summary>Array of results (up to MAXNOOFTABLES * DDS_STRAINS = 200).</summary>
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = DdsConstants.MAXNOOFTABLES * DdsConstants.DDS_STRAINS)]
    public DdTableResults[] results;

    /// <summary>
    /// Safe indexer with bounds checking against actual no_of_boards.
    /// </summary>
    public DdTableResults this[int index]
    {
        get
        {
            if (index <  0 || index >= no_of_boards)
                throw new IndexOutOfRangeException($"Index {index} out of range [0, {no_of_boards - 1}]");

            return results[index];
        }

        set
        {
            if (index <  0 || index >= no_of_boards)
                throw new IndexOutOfRangeException($"Index {index} out of range [0, {no_of_boards - 1}]");

            results[index] = value;
        }
    }
}
