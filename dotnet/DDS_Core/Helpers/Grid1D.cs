using System;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// Generic 1D array wrapper for marshalling with bounds checking.
/// </summary>
public struct Grid1D<T> where T : unmanaged
{
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 0)]  // Placeholder, will be overridden
    private T[] data;

    private readonly int size;

    public Grid1D(int size)
    {
        if (size<=0)
            throw new ArgumentException("Size must be positive", nameof(size));

        this.size =size;
        this.data =new T[size];
    }

    /// <summary>Gets or sets an element with bounds checking.</summary>
    public ref T this[int index]
    {
        get
        {
            if (index<0||index>=size)
                throw new IndexOutOfRangeException($"Index {index} out of range [0, {size-1}]");

            return ref data[index];
        }
    }

    /// <summary>Number of elements.</summary>
    public int     Length => size;

    /// <summary>Gets the underlying array as a span.</summary>
    public Span<T> AsSpan() => new(data, 0, size);
}
