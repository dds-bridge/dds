using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Multiple deals for batch double dummy table calculation.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct DdTableDeals
{
    /// <summary>Number of tables.</summary>
    public int NumberOfTables;

    /// <summary>Array of deals (up to MAXNOOFTABLES * DDS_STRAINS).</summary>
    public DdTableDealsArray Deals;

    
    #region Nested Types

        [InlineArray(DdsConstants.DdsStrains)]
        public struct DdTableDealsArray
        {
            private DdTableDeal item;

            public static implicit operator DdTableDealsArray(DdTableDeal[] array)
            {
                var result = new DdTableDealsArray();

                if (array != null)
                {
                    var span = result.AsSpan();

                    for (int i = 0; i <  Math.Min(array.Length, span.Length); i++)
                        span[i] = array[i];
                }

                return result;
            }

            // Implicit conversion from DdTableDealsArray to Span<DdTableDeal>
            private Span<DdTableDeal> AsSpan()
            {
                return System.Runtime.InteropServices.MemoryMarshal.CreateSpan(ref item, DdsConstants.DdsStrains);
            }
        }
    #endregion
}
