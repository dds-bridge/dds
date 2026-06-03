using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Solutions for multiple boards.
/// 
/// Container for results from batch board solving operations.
/// Each entry contains the complete future tricks analysis for one board.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct SolvedBoards
{
    /// <summary>Number of solved boards.</summary>
    public int NumberOfBoards;

    /// <summary>Array of solutions (future tricks for each board).</summary>
    public FutureTricksArray Tricks;

    #region Nested Types
        [InlineArray(DdsConstants.MaxNumberOfBoards)]
        public struct FutureTricksArray
        {
            private FutureTricks item;

            public static implicit operator FutureTricksArray(FutureTricks[] array)
            {
                var result = new FutureTricksArray();

                if (array != null)
                {
                    var span = result.AsSpan();

                    for (int i = 0; i <  Math.Min(array.Length, span.Length); i++)
                        span[i] = array[i];
                }

                return result;
            }

            // Implicit conversion from FutureTricksArray to Span<FutureTricks>
            private Span<FutureTricks> AsSpan()
            {
                return System.Runtime.InteropServices.MemoryMarshal.CreateSpan(ref item, DdsConstants.MaxNumberOfBoards);
            }
        }
    #endregion
}
