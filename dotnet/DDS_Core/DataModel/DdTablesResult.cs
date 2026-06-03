using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Multiple double dummy table results.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct DdTablesResult
{
    /// <summary>Number of boards.</summary>
    public int no_of_boards;

    /// <summary>Array of results (up to MAXNOOFTABLES * DDS_STRAINS = 200).</summary>
    public DdTableResultsArray Results;

    /// <summary>
    /// Safe indexer with bounds checking against actual no_of_boards.
    /// </summary>
    public DdTableResults this[int index]
    {
        get
        {
            if (index <  0 || index >= no_of_boards)
                throw new IndexOutOfRangeException($"Index {index} out of range [0, {no_of_boards - 1}]");

            return Results[index];
        }

        set
        {
            if (index <  0 || index >= no_of_boards)
                throw new IndexOutOfRangeException($"Index {index} out of range [0, {no_of_boards - 1}]");

            Results[index] = value;
        }
    }

    #region Nested Types
        [InlineArray(DdsConstants.MaxNumberOfTables * DdsConstants.DdsStrains)]
        public struct DdTableResultsArray
        {
            private DdTableResults item;

            public static implicit operator DdTableResultsArray(DdTableResults[] array)
            {
                var result = new DdTableResultsArray();

                if (array != null)
                {
                    var span = result.AsSpan();

                    for (int i = 0; i <  Math.Min(array.Length, span.Length); i++)
                        span[i] = array[i];
                }

                return result;
            }

            // Implicit conversion from DdTableResultsArray to Span<DdTableResults>
            private Span<DdTableResults> AsSpan()
            {
                return System.Runtime.InteropServices.MemoryMarshal.CreateSpan(ref item, DdsConstants.MaxNumberOfTables * DdsConstants.DdsStrains);
            }
        }
    #endregion
}
