using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace DDS_Core;

[DebuggerDisplay("{ToString(),nq}")]
[DebuggerTypeProxy(typeof(intArray5xDebugView<intArray5x4>))]
[StructLayout(LayoutKind.Sequential)]
public unsafe struct intArray5x4 : IBuffer
{
    public const int ROWS = 5;
    public const int COLS = 4;
    public const int SIZE = ROWS * COLS;

    private fixed int data[SIZE];

    public Span<int> AsSpan()
                        => MemoryMarshal.CreateSpan(ref data[0], SIZE);

    public Span<int> RowAsSpan(int row)
    {
        if (row >= ROWS || row <  0)
            throw new ArgumentOutOfRangeException(nameof(row));

        // ref to first byte in the array for the specified row
        ref int start = ref data[row * COLS];

        return MemoryMarshal.CreateSpan(ref start, COLS);
    }

    public Span<int> this[int row]
    {
        get
        {
            if ((uint)row >= ROWS)
                throw new IndexOutOfRangeException();

            // ref to the first element in the specified row
            ref int start = ref data[row * COLS];

            // correct way to create a span over the fixed buffer
            return MemoryMarshal.CreateSpan(ref start, COLS);
        }
    }

    public ref int this[int row, int col]
    {
        get
        {
            if (row >= ROWS || row <  0
            ||  col >= COLS || col         <  0)
                throw new IndexOutOfRangeException();

            return ref data[row * COLS + col];
        }
    }

    public static implicit operator intArray5x4(int[] src)
    {
        var buf = new intArray5x4();

        for (int c = 0; c <  16; c++)
            buf.data[c] = src[c];

        return buf;
    }

    public static implicit operator intArray5x4(int[][] src)
    {
        var buf = new intArray5x4();

        int k = 0;

        for (int r = 0; r <  4; r++)
            for (int c = 0; c <  4; c++)
                buf.data[k++] = src[r][c];

        return buf;
    }

    public string GetString(int row)
            => string.Join(",", RowAsSpan(row).ToArray());

    //public override string ToString()
    //        => "["+string.Join("],[", Enumerable.Range(0, ROWS).Select(GetString)) +"]";
    public override string ToString()
    {
        var parts = new string[ROWS];

        for (int r = 0; r <  ROWS; r++)
        {
            var row  = RowAsSpan(r);
            parts[r] = "[" + string.Join(",", row.ToArray()) + "]";
        }

        return string.Join(" | ", parts);
    }
}
