using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace DDS_Core;

#region Native structures for DDS.dll
    // Do not set [... , Pack = 1]
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct Deal
    {
        public int trump;
        public int first;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
        public int[] currentTrickSuit;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
        public int[] currentTrickRank;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public uint[] remainCards;
    }
#endregion
