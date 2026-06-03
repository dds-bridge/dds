using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Tricks per strain and hand using 2D indexing.
/// Access: res_table[strain, hand] where strain=0-4, hand=0-3
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public unsafe struct ResultsTable
{
    public const int ROWS = 5;
    public const int COLS = 4;
    public const int SIZE = ROWS * COLS;

    private fixed int data[SIZE];

    public Span<int> AsSpan()
        => MemoryMarshal.CreateSpan(ref data[0], SIZE);

    public Span<int> RowAsSpan(int row)
    {
        if ((uint)row >= ROWS)
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
            if ((uint)row >= ROWS || (uint)col >= COLS)
                throw new IndexOutOfRangeException();

            return ref data[row * COLS + col];
        }
    }
}


