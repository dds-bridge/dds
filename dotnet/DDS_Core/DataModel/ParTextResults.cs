using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Text representation of par results.
/// 
/// Includes short text summary and information about equality.
/// </summary>
[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct ParTextResults
{
    /// <summary>
    /// Short par text for NS and EW using 2D indexing (2 x 128 chars).
    /// Access: par_text[side, index] where side=0(NS) or 1(EW)
    /// </summary>
    public ByteBuffer2x128 par_text;

    /// <summary>True if equal (doesn't matter who starts bidding), false otherwise.</summary>
    [MarshalAs(UnmanagedType.I1)]
    public bool equal;
}
