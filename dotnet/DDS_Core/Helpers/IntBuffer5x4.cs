using System.Runtime.InteropServices;

namespace DDS_Core
{
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct IntBuffer5x4
    {
        public const int ROWS = 5;
        public const int COLS = 4;
        public const int SIZE = ROWS * COLS;

        public fixed int data[SIZE];

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
}


