using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Multiple deals in PBN format for batch double dummy table calculation.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct DdTableDealsPBN
{
    /// <summary>Number of tables.</summary>
    public int NumberOfTables;

    /// <summary>Array of PBN deals (up to MAXNOOFTABLES * DDS_STRAINS).</summary>
    public DdTableDealsPBNArray Deals;
    #region Nested Types

        [InlineArray(DdsConstants.DdsStrains)]
        public struct DdTableDealsPBNArray
        {
            private DdTableDealPBN item;

            public static implicit operator DdTableDealsPBNArray(DdTableDealPBN[] array)
            {
                var result = new DdTableDealsPBNArray();

                if (array != null)
                {
                    var span = result.AsSpan();

                    for (int i = 0; i <  Math.Min(array.Length, span.Length); i++)
                        span[i] = array[i];
                }

                return result;
            }

            // Implicit conversion from DdTableDealsPBNArray to Span<DdTableDealPBN>
            private Span<DdTableDealPBN> AsSpan()
            {
                return System.Runtime.InteropServices.MemoryMarshal.CreateSpan(ref item, DdsConstants.DdsStrains);
            }
        }
    #endregion
}
