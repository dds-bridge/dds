using System;

namespace DDS_Core
{
    internal sealed class intArray5xDebugView<T> where T : IBuffer
    {
        private readonly T _buffer;

        public intArray5xDebugView(T buffer)
        {
            _buffer = buffer;
        }

        public string Row0 => _buffer.GetString(0);
        public string Row1 => _buffer.GetString(1);
        public string Row2 => _buffer.GetString(2);
        public string Row3 => _buffer.GetString(3);
        public string Row4 => _buffer.GetString(4);

    }
}

