using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// A single deal in PBN format for double dummy table calculation.
/// 
/// Uses PBN (Portable Bridge Notation) string representation.
/// </summary>
[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct DdTableDealPBN
{
    /// <summary>Cards in PBN format (max 80 characters).</summary>
    public string80 Cards;
}
