using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Double dummy results for all strains and hands.
/// 
/// Results indexed by strain (0-4: S/H/D/C/NT) and hand (0-3: N/E/S/W).
/// Each entry contains the number of tricks that hand can take in that strain.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct DdTableResults
{
    /// <summary>
    /// Tricks per strain and hand using 2D indexing.
    /// Access: res_table[strain, hand] where strain=0-4, hand=0-3
    /// </summary>
    public intArray5x4 ResultsTable;
}
