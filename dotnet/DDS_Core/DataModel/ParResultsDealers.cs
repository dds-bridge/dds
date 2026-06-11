using System;
using System.Runtime.InteropServices;

namespace DDS_Core
{

    /// <summary>
    /// ParResultsDealer[2]
    /// 
    /// Contains number of par contracts and their details.
    /// </summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct ParResultsDealers
    {
        private ParResultsDealer parResultsDealersNS;
        private ParResultsDealer parResultsDealersEW;

        public ParResultsDealer this[int index]
        {
            get
            {
                if ((uint)index >= 2)
                    throw new IndexOutOfRangeException();

                if (index == 0)
                    return parResultsDealersNS;
                else
                    return parResultsDealersEW;
            }

            set
            {
                if ((uint)index >= 2)
                    throw new IndexOutOfRangeException();

                if (index == 0)
                    parResultsDealersNS = value;
                else
                    parResultsDealersEW = value;
            }
        }
    }
}
