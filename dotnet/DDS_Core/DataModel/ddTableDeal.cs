using System.Runtime.InteropServices;

namespace DDS_Core
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct ddTableDeal
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        public int[] trump;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        public int[] first;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public uint[] remainCards;
    }
}
