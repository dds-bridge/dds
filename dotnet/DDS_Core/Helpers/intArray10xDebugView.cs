using System;

namespace DDS_Core
{
    internal sealed class intArray10xDebugView<T> where T : IBuffer
    {
        private readonly T _buffer;

        public intArray10xDebugView(T buffer)
        {
            _buffer = buffer;
        }

        public string Row0 => _buffer.GetString(0);
        public string Row1 => _buffer.GetString(1);
        public string Row2 => _buffer.GetString(2);
        public string Row3 => _buffer.GetString(3);

        public string Row4 => _buffer.GetString(4);
        public string Row5 => _buffer.GetString(5);

        public string Row6 => _buffer.GetString(6);
        public string Row7 => _buffer.GetString(7);

        public string Row8 => _buffer.GetString(8);
        public string Row9 => _buffer.GetString(9);
    }
}
