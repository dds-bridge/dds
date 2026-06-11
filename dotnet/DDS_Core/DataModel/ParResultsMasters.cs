using System;
using System.Runtime.InteropServices;

namespace DDS_Core
{

    /// <summary>
    /// Par contracts for both dealer and vulnerable combinations.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]

    public struct ParResultsMasters
    {
        /// <summary>InlineArray of 10 par contracts.</summary>
        private ParResultsMaster parResultsMaster0;
        private ParResultsMaster parResultsMaster1;

        public ParResultsMaster this[int index]
        {
            get
            {
                if ((uint)index >= 2)
                    throw new IndexOutOfRangeException();

                if (index == 0)
                    return parResultsMaster0;
                else
                    return parResultsMaster1;
            }

            set
            {
                if ((uint)index >= 2)
                    throw new IndexOutOfRangeException();

                if (index == 0)
                    parResultsMaster0 = value;
                else
                    parResultsMaster1 = value;
            }
        }
    }
}
