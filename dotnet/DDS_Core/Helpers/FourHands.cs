using System.Runtime.InteropServices;

namespace DDS_Core;

[StructLayout(LayoutKind.Sequential)]
public unsafe struct FourHands
{
    public const int ROWS = 4;
    public const int COLS = 4;
    public const int SIZE = ROWS * COLS;

    private fixed uint data[SIZE];

    public Span<uint> AsSpan()
                => MemoryMarshal.CreateSpan(ref data[0], SIZE);

    public Span<uint> RowAsSpan(int row)
    {
        if ((uint)row >= ROWS)
            throw new ArgumentOutOfRangeException(nameof(row));

        // ref to first byte in the array for the specified row
        ref uint start = ref data[row * COLS];

        return MemoryMarshal.CreateSpan(ref start, COLS);
    }

    public Span<uint> this[int row]
    {
        get
        {
            if ((uint)row >= ROWS)
                throw new IndexOutOfRangeException();

            // ref to the first element in the specified row
            ref uint start = ref data[row * COLS];

            // correct way to create a span over the fixed buffer
            return MemoryMarshal.CreateSpan(ref start, COLS);
        }
    }

    public ref uint this[int row, int col]
    {
        get
        {
            if ((uint)row >= ROWS || (uint)col >= COLS)
                throw new IndexOutOfRangeException();

            return ref data[row * COLS + col];
        }
    }

    public static implicit operator FourHands(uint[] src)
    {
        var buf = new FourHands();

        for (int c = 0; c <  16; c++)
            buf.data[c] = src[c];

        return buf;
    }

    public static implicit operator FourHands(uint[][] src)
    {
        var buf = new FourHands();

        int k = 0;

        for (int r = 0; r <  4; r++)
            for (int c = 0; c <  4; c++)
                buf.data[k++] = src[r][c];

        return buf;
    }
}
