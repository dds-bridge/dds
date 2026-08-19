using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Multiple boards in PBN format for batch solving.
/// 
/// Similar to Boards but uses PBN (Portable Bridge Notation) format
/// for deal representation. Used for solving multiple boards efficiently.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct BoardsPBN
{
    /// <summary>Number of boards to solve.</summary>
    public int NumberOfBoards;

    /// <summary>Array of deals in PBN format.</summary>
    public DealsPBNArray Deals;

    /// <summary>Target tricks for each board.</summary>
    public intArray200 Target;

    /// <summary>Solution mode for each board.</summary>
    public intArray200 Solutions;

    /// <summary>Solve mode for each board.</summary>
    public intArray200 Modes;

    #region Nested Types
        [InlineArray(DdsConstants.MaxNumberOfBoards)]
        public struct DealsPBNArray
        {
            private DealPBN item;

            public static implicit operator DealsPBNArray(DealPBN[] array)
            {
                var result = new DealsPBNArray();

                if (array != null)
                {
                    var span = result.AsSpan();

                    for (int i = 0; i <  Math.Min(array.Length, span.Length); i++)
                        span[i] = array[i];
                }

                return result;
            }

            // Implicit conversion from DealsPBNArray to Span<DealPBN>
            private Span<DealPBN> AsSpan()
            {
                return System.Runtime.InteropServices.MemoryMarshal.CreateSpan(ref item, DdsConstants.MaxNumberOfBoards);
            }
    #endregion
    }
}
