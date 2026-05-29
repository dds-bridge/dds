using System;
using System.Runtime.InteropServices;

namespace DDS_Core
{
    /// <summary>
    /// Par result for a specific dealer.
    /// 
    /// Contains number of par contracts and their details.
    /// </summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct ParResultsDealer
    {
        /// <summary>Number of contracts yielding the par score.</summary>
        public int number;

        /// <summary>Par score for the specified dealer hand.</summary>
        public int score;

        /// <summary>Par contract text strings (10 entries, max 10 chars each).</summary>
        //[MarshalAs(UnmanagedType.ByValArray, SizeConst = 100)]
        public ByteBuffer100 contracts;
    }
}
