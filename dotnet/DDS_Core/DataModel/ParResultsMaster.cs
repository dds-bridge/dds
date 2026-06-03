using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Par contracts for both dealer and vulnerable combinations.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct ParResultsMaster
{
    /// <summary>Par score (sign according to NS view).</summary>
    public int Score;

    /// <summary>Number of contracts giving the par score.</summary>
    public int Number;

    /// <summary>InlineArray of 10 par contracts.</summary>
    public ContractTypes Contracts;

    #region Nested Types
        [InlineArray(10)]
        public struct ContractTypes
        {
            private ContractType Contract;
        }
    #endregion
}
