using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Par results for all declarer/strain combinations (up to 40).
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct AllParResults
{
    /// <summary>Array of par results (up to MAXNOOFTABLES entries).</summary>
    public ParResultsArray ParResults;

    #region Nested Types
    [InlineArray(DdsConstants.MaxNumberOfTables)]
        public struct ParResultsArray
        {
            private ParResults item;
        }
    #endregion

}
