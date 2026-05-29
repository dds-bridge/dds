using System;
using System.Runtime.InteropServices;

namespace DDS_Core;

[StructLayout(LayoutKind.Sequential)]
public unsafe struct ByteBuffer2x128
{
    public const int ROWS = 2;
    public const int COLS = 128;
    public const int SIZE = ROWS * COLS;

    public fixed byte data[SIZE];

    public Span<byte> AsSpan()
        => MemoryMarshal.CreateSpan(ref data[0], SIZE);

    public Span<byte> RowAsSpan(int row)
    {
        if ((uint)row >= ROWS)
            throw new ArgumentOutOfRangeException(nameof(row));

        // ref to first byte in the array for the specified row
        ref byte start = ref data[row * COLS];

        return MemoryMarshal.CreateSpan(ref start, COLS);
    }

    public ref byte this[int row, int col]
    {
        get
        {
            if ((uint)row >= ROWS || (uint)col >= COLS)
                throw new IndexOutOfRangeException();

            return ref data[row * COLS + col];
        }
    }
}
