using System;
using System.Runtime.InteropServices;

namespace DDS_Core;

[StructLayout(LayoutKind.Sequential)]
public unsafe struct ByteBuffer32
{
    public const int SIZE = 32;

    public fixed byte data[SIZE];

    public Span<byte> AsSpan() => MemoryMarshal.CreateSpan(ref data[0], SIZE);

    public ref byte this[int index]
    {
        get
        {
            if ((uint)index >= SIZE)
                throw new IndexOutOfRangeException();

            return ref data[index];
        }
    }
}
