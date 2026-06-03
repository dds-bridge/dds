using System;

namespace DDS_Core;

internal sealed class intArray2xDebugView<T> where T : IBuffer
{
    private readonly T _buffer;

    public intArray2xDebugView(T buffer)
    {
        _buffer = buffer;
    }

    public string Row0 => _buffer.GetString(0);
    public string Row1 => _buffer.GetString(1); 
}
