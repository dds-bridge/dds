using System.Runtime.InteropServices;

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct ddTableResults
{
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 20)]
    public int[] resTable;
}
