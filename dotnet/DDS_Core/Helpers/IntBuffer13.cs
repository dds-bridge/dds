using System;
using System.Runtime.InteropServices;

namespace DDS_Core
{
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct IntBuffer13
    {
        public const int SIZE = 13;

        public fixed int data[SIZE];

        public Span<int> AsSpan() => MemoryMarshal.CreateSpan(ref data[0], SIZE);

        public ref int this[int index]
        {
            get
            {
                if ((uint)index >= SIZE)
                    throw new IndexOutOfRangeException();

                return ref data[index];
            }
        }
    }
}
