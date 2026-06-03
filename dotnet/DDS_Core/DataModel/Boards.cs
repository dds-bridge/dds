using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Represents multiple bridge deals for batch analysis.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct Boards
{
    /// <summary>Number of deals in this batch.</summary>
    public int NumberOfBoards;

    /// <summary>Array of deals.</summary>
    public DealsArray Deals;

    /// <summary>Target tricks for each board.</summary>
    public intArray200 Target;

    /// <summary>Solution mode for each board (1=best, 2=all, 3=all+par).</summary>
    
    public intArray200 Solutions;

    /// <summary>Solve mode for each board.</summary>
    public intArray200 Modes;

    #region Nested Types
        [InlineArray(DdsConstants.MaxNumberOfBoards)]
        public struct DealsArray
        {
            private Deal item;

            public static implicit operator DealsArray(Deal[] array)
            {
                var result = new DealsArray();

                if (array != null)
                {
                    var span = result.AsSpan();

                    for (int i = 0; i <  Math.Min(array.Length, span.Length); i++)
                        span[i] = array[i];
                }

                return result;
            }

            // Implicit conversion from DealsArray to Span<Deal>
            private Span<Deal> AsSpan()
            {
                return System.Runtime.InteropServices.MemoryMarshal.CreateSpan(ref item, DdsConstants.MaxNumberOfBoards);
            }
        }
    #endregion
}
