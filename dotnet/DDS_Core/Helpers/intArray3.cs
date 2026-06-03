using System;
using System.Linq;
using System.Runtime.CompilerServices;

namespace DDS_Core;

[InlineArray(3)]
public struct intArray3
{
    private int item;

    public static implicit operator intArray3(int[] src)
    {
        var buf  = new intArray3();
        var span = buf.AsSpan();

        int count = Math.Min(span.Length, src.Length);

        for (int i = 0; i < count; i++)
            span[i] = src[i];

        for (int i = count; i < span.Length; i++)
            span[i] = 0;

        return buf;
    }

    public Span<int> AsSpan()
            => System.Runtime.InteropServices.MemoryMarshal.CreateSpan(ref item, 3);

}
