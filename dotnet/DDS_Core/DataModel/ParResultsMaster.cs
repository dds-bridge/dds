using System;
using System.Runtime.InteropServices;

namespace DDS_Core
{

    /// <summary>
    /// Par contracts for both dealer and vulnerable combinations.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct ParResultsMaster
    {
        /// <summary>Par score (sign according to NS view).</summary>
        public int score;

        /// <summary>Number of contracts giving the par score.</summary>
        public int number;

        /// <summary>Array of par contracts (up to 10).</summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 10)]
        public ContractType[] contracts;
    }
}
