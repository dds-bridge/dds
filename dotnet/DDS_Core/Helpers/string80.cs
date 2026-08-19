using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace DDS_Core;

/// <remarks>
/// Byte buffer struct is suitable for interop with native code, but in c# it acts 
/// like a simple string with fixed max length per string. Strings are ASCII-encoded 
/// and padded with nulls if shorter than max length.
/// </remarks>
[DebuggerDisplay("{ToString()}")]
[StructLayout(LayoutKind.Sequential)]
public unsafe struct string80
{
    public const int SIZE = 80;

    private fixed byte data[SIZE];

    // -------------------------------
    // Indexers
    // -------------------------------
    public ref byte this[int index]
    {
        get
        {
            if ((uint)index >= SIZE)
                throw new IndexOutOfRangeException();

            return ref data[index];
        }
    }

    public string Value
    {
        get => GetString();
        set => SetString(value);
    }

    // -------------------------------
    // String assignment API
    // -------------------------------
    public void SetString(string value)
    {
        if (value is null)
            throw new ArgumentNullException(nameof(value));

        if (value.Length > SIZE)
            throw new ArgumentOutOfRangeException(nameof(value));

        ref byte start = ref data[0];
        var span = MemoryMarshal.CreateSpan(ref start, SIZE);

        // ASCII encode
        int written = Encoding.ASCII.GetBytes(value, span);

        if (written < SIZE)
            span.Slice(written).Clear();
    }

    public string GetString()
    {
        ref byte start = ref data[0];
        var span = MemoryMarshal.CreateSpan(ref start, SIZE);
        return Encoding.ASCII.GetString(span).TrimEnd('\0');
    }

    public Span<byte> AsSpan()
        => MemoryMarshal.CreateSpan(ref data[0], SIZE);

    // -------------------------------
    // Implicit conversion 
    // -------------------------------
    public static implicit operator string80(string src)
    {
        var buf = new string80();
        buf.SetString(src);
        return buf;
    }

    // -------------------------------
    // Debugger / ToString
    // -------------------------------
    public override string ToString() => GetString();
}
