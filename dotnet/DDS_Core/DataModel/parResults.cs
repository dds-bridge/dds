using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Par score and contracts for a single declarer/strain combination.
/// 
/// Includes both NS and EW perspectives.
/// Index 0 = NS view, Index 1 = EW view.
/// </summary>
[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct ParResults
{
    /// <summary>
    /// Par score strings for NS and EW (2 entries, max 16 chars each).
    /// Access: par_score[side, index] where side=0(NS) or 1(EW)
    /// </summary>
    public intArray2x16 ParScores;


    /// <summary>
    /// Par contract strings for NS and EW (2 entries, max 128 chars each).
    /// Access: par_contracts_string[side, index] where side=0(NS) or 1(EW)
    /// </summary>
    public intArray2x128 ParContractStrings;
}
