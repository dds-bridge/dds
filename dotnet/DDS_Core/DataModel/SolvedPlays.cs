using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Analyzed results of multiple play sequences.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct SolvedPlays
{
    /// <summary>Number of solved plays.</summary>
    public int NumberOfPlayedBoards;

    /// <summary>Array of solved play results (up to MAXNOOFBOARDS).</summary>
    //[MarshalAs(UnmanagedType.ByValArray, SizeConst = DdsConstants.MaxNumberOfBoards)]
    //public SolvedPlay[] Solved;
    public SolvedPlayArray Solved;

    #region Nested Types
        [InlineArray(DdsConstants.MaxNumberOfBoards)]
        public struct SolvedPlayArray
        {
            private SolvedPlay item;

            public static implicit operator SolvedPlayArray(SolvedPlay[] array)
            {
                var result = new SolvedPlayArray();

                if (array != null)
                {
                    var span = result.AsSpan();

                    for (int i = 0; i <  Math.Min(array.Length, span.Length); i++)
                        span[i] = array[i];
                }

                return result;
            }

            // Implicit conversion from SolvedPlayArray to Span<SolvedPlay>
            private Span<SolvedPlay> AsSpan()
            {
                return System.Runtime.InteropServices.MemoryMarshal.CreateSpan(ref item, DdsConstants.MaxNumberOfBoards);
            }
        }
    #endregion
}
