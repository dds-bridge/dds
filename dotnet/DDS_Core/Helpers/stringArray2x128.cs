using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace DDS_Core;


/// <remarks>
/// Byte buffer struct is suitable for interop with native code, but in c# it acts 
/// like a 2D string array with fixed max length per string. Strings are ASCII-encoded 
/// and padded with nulls if shorter than max length.
/// </remarks>
[DebuggerDisplay("{ToString()}")]
[DebuggerTypeProxy(typeof(stringArray2xDebugView<stringArray2x128>))]
[StructLayout(LayoutKind.Sequential)]
public unsafe struct stringArray2x128 : IBuffer
{
    public const int ROWS = 2;
    public const int COLS = 128;
    public const int SIZE = ROWS * COLS;

    private fixed byte data[SIZE];

    #region Indexers
        public ref byte this[int row, int col]
        {
            get
            {
                if ((uint)row >= ROWS || (uint)col >= COLS)
                    throw new IndexOutOfRangeException();

                return ref data[row * COLS + col];
            }
        }

        public string this[int row]
        {
            get => GetString(row);
            set => SetString(row, value);
        }
    #endregion

    #region String assignment API
        public void SetString(int row, string value)
        {
            if ((uint)row >= ROWS)
                throw new ArgumentOutOfRangeException(nameof(row));

            if (value is null)
                throw new ArgumentNullException(nameof(value));

            ref byte start = ref data[row * COLS];
            var      span  = MemoryMarshal.CreateSpan(ref start, COLS);

            // encode to ASCII
            int written = Encoding.ASCII.GetBytes(value, span);

            if (written <  COLS)
                span.Slice(written).Clear();
        }

        public string GetString(int row)
        {
            if ((uint)row >= ROWS)
                throw new ArgumentOutOfRangeException(nameof(row));

            ref byte start = ref data[row * COLS];
            var      span  = MemoryMarshal.CreateSpan(ref start, COLS);
            return Encoding.ASCII.GetString(span).TrimEnd('\0');
        }
    #endregion

    #region Spans
        public Span<byte> AsSpan() => MemoryMarshal.CreateSpan(ref data[0], SIZE);

        public Span<byte> RowAsSpan(int row)
        {
            if ((uint)row >= ROWS)
                throw new ArgumentOutOfRangeException(nameof(row));

            // ref to first byte in the array for the specified row
            ref byte start = ref data[row * COLS];

            return MemoryMarshal.CreateSpan(ref start, COLS);
        }
    #endregion

        public override string ToString()
            => "["+string.Join("],[", Enumerable.Range(0, ROWS).Select(GetString)) +"]";
}
